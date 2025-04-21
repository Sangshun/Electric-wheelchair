#include "pir_sensor.hpp"
#include <iostream>
#include <pthread.h>

using namespace std::chrono_literals;


PIRController::PIRController(
    const std::string& chip_name,
    unsigned int pin,
    MotionCallback cb
) : m_pin(pin), m_callback(cb) 
{
    try {

        m_chip = gpiod::chip(chip_name);


        m_line = m_chip.get_line(m_pin);


        gpiod::line_request config = {
            "PIR-Sensor", 
            gpiod::line_request::EVENT_BOTH_EDGES,
            0
        };
        m_line.request(config);

    } catch (const std::system_error& e) {
 
        std::cerr << "[PIR] System Error: " << e.what() 
                  << " (code: " << e.code() << ")\n";
        throw; 

    } catch (const std::exception& e) {
   
        std::cerr << "[PIR] Initialization Error: " << e.what() << "\n";
        throw std::runtime_error("PIR controller initialization failed");
        
    } catch (...) {

        std::cerr << "[PIR] Unknown error during initialization\n";
        throw;
    }
}


PIRController::~PIRController() {
    stop(); 
    try {
        if (m_line.is_requested()) {
            m_line.release(); 
        }
    } catch (const std::exception& e) {
        std::cerr << "[PIR] Error releasing line: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[PIR] Unknown error releasing line\n";
    }
}


void PIRController::start(unsigned int debounce_ms) {
    if (m_running) return;

    m_debounce_time = std::chrono::milliseconds(debounce_ms);
    m_running = true;

    try {

        m_monitor_thread = std::thread(&PIRController::event_monitor, this);


        sched_param params{};
        params.sched_priority = sched_get_priority_max(SCHED_FIFO);
        if (pthread_setschedparam(m_monitor_thread.native_handle(), 
                                 SCHED_FIFO, &params)) {
            std::cerr << "[PIR] Warning: Failed to set real-time priority\n";
        }

    } catch (const std::system_error& e) {
        m_running = false;
        std::cerr << "[PIR] Thread Error: " << e.what() 
                  << " (code: " << e.code() << ")\n";
        throw;

    } catch (...) {
        m_running = false;
        std::cerr << "[PIR] Failed to start monitoring thread\n";
        throw;
    }
}

void PIRController::stop() noexcept {
    m_running = false;
    try {
        if (m_monitor_thread.joinable()) {
            try {
                m_monitor_thread.join(); 
            } catch (const std::system_error& e) {
                std::cerr << "[PIR] Thread Join Error: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[PIR] Unknown error joining thread\n";
            }
        }
    } catch (...) {
       
    }
}


void PIRController::set_callback(MotionCallback new_cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_callback = new_cb;
}


void PIRController::event_monitor() {
    using clock = std::chrono::steady_clock;
    auto last_event_time = clock::now();

    while (m_running) {
        try {
            
            if (m_line.event_wait(100ms)) {
                const auto now = clock::now();

                
                if (now - last_event_time < m_debounce_time) continue;
                last_event_time = now;

               
                gpiod::line_event event = m_line.event_read();
                const bool current_state = 
                    (event.event_type == gpiod::line_event::RISING_EDGE);

              
                if (current_state != m_last_state.exchange(current_state)) {
                    std::lock_guard<std::mutex> lock(m_callback_mutex);
                    if (m_callback) {
                        try {
                            m_callback(current_state); 
                        } catch (const std::exception& e) {
                            std::cerr << "[PIR] Callback Error: " 
                                      << e.what() << "\n";
                        } catch (...) {
                            std::cerr << "[PIR] Unknown callback error\n";
                        }
                    }
                }
            }

        } catch (const std::system_error& e) {
            if (e.code().value() == EINTR) { 
                continue;
            }
            std::cerr << "[PIR] Monitoring Error: " << e.what()
                      << " (code: " << e.code() << ")\n";
            break;

        } catch (const std::exception& e) {
            std::cerr << "[PIR] Monitoring Error: " << e.what() << "\n";
            break;

        } catch (...) {
            std::cerr << "[PIR] Unknown monitoring error\n";
            break;
        }
    }


    m_running = false;
}
