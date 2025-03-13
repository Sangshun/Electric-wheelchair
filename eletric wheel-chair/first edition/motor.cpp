#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <gpiod.hpp>
#include <csignal>

// GPIO configuration constants
constexpr char GPIO_CHIP[] = "gpiochip0";
constexpr unsigned IN1_PIN = 17;
constexpr unsigned IN2_PIN = 27;

// Motor operation states
enum class MotorState { STOP, FORWARD, BACKWARD };

// Forward declarations
class MotorController;
class CommandQueue;

class MotorController {
public:
    explicit MotorController();
    void set_state(MotorState new_state);
    void emergency_stop();
    MotorState get_state() const;
    ~MotorController();

private:
    void update_gpio();

    // GPIO resources
    gpiod::chip m_chip;
    gpiod::line m_in1;
    gpiod::line m_in2;
    
    // Thread safety
    mutable std::mutex m_mutex;
    std::atomic<MotorState> m_current_state;
    std::atomic_bool m_emergency_flag;
};

MotorController::MotorController() : 
    m_current_state(MotorState::STOP),
    m_emergency_flag(false) 
{
    try {
        m_chip = gpiod::chip(GPIO_CHIP);
        m_in1 = m_chip.get_line(IN1_PIN);
        m_in2 = m_chip.get_line(IN2_PIN);
        
        gpiod::line_request config = {
            "motor_ctrl",
            gpiod::line_request::DIRECTION_OUTPUT,
            0
        };
        
        m_in1.request(config, 0);
        m_in2.request(config, 0);
    } catch (const std::exception& e) {
        std::cerr << "GPIO initialization failed: " << e.what() << std::endl;
        throw;
    }
}

void MotorController::set_state(MotorState new_state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_emergency_flag.load()) return;

    m_current_state = new_state;
    update_gpio();
}


void MotorController::emergency_stop() {
    m_emergency_flag.store(true);
    set_state(MotorState::STOP);
}

void MotorController::update_gpio() {
    switch(m_current_state.load()) {
        case MotorState::FORWARD:
            m_in1.set_value(1);
            m_in2.set_value(0);
            break;
        case MotorState::BACKWARD:
            m_in1.set_value(0);
            m_in2.set_value(1);
            break;
        case MotorState::STOP:
        default:
            m_in1.set_value(0);
            m_in2.set_value(0);
    }
}

MotorController::~MotorController() {
    m_in1.release();
    m_in2.release();
}

class CommandQueue {
public:
    void push(char cmd);
    char pop();

private:
    std::queue<char> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

void CommandQueue::push(char cmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(cmd);
    m_cv.notify_one();
}

char CommandQueue::pop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]{ return !m_queue.empty(); });
    
    char cmd = m_queue.front();
    m_queue.pop();
    return cmd;
}

class Application {
public:
    Application();
    void run();

private:
    void setup_signal_handler();
    void start_input_thread();
    void process_commands();
    void handle_command(char cmd);

    std::unique_ptr<MotorController> m_motor;
    CommandQueue m_queue;
    std::thread m_input_thread;
    static std::atomic_bool s_running;
};

std::atomic_bool Application::s_running{true};

Application::Application() : 
    m_motor(std::make_unique<MotorController>()) {}

void Application::setup_signal_handler() {
    std::signal(SIGINT, [](int){ s_running = false; });
}

void Application::start_input_thread() {
    m_input_thread = std::thread([this]{
        while(s_running) {
            std::cout << "Enter command (f/b/s/e/q): ";
            char cmd;
            std::cin >> cmd;
            
            if (cmd == 'q') {
                s_running = false;
                break;
            }
            m_queue.push(cmd);
        }
    });
}

void Application::process_commands() {
    while(s_running) {
        const char cmd = m_queue.pop();
        handle_command(cmd);
    }
    
    if (m_input_thread.joinable()) {
        m_input_thread.join();
    }
}

void Application::handle_command(char cmd) {
    switch(cmd) {
        case 'f':
            m_motor->set_state(MotorState::FORWARD);
            std::cout << "Motor forward\n";
            break;
        case 'b':
            m_motor->set_state(MotorState::BACKWARD);
            std::cout << "Motor backward\n";
            break;
        case 's':
            m_motor->set_state(MotorState::STOP);
            std::cout << "Motor stopped\n";
            break;
        case 'e':
            m_motor->emergency_stop();
            std::cout << "Emergency stop activated\n";
            break;
        default:
            std::cerr << "Invalid command: " << cmd << std::endl;
    }
}

void Application::run() {
    setup_signal_handler();
    start_input_thread();
    process_commands();
}

int main() {
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
