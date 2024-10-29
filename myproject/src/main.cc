#include<stdbool.h> //for bool
#include<unistd.h> //for usleep
#include <math.h>
#include <iostream>

#include "mujoco/mujoco.h"
#include "GLFW/glfw3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MCLmodel.hpp"
#include "MuJoCoMessageHandler.hpp"
#include <mutex>
//#include <Eigen/Core>
//#include <Eigen/Dense>

#define NUMOFMOTOR 12
#define SHM_NAME "/mcl_420"
#define SHM_SIZE sizeof(SharedData)
#define RT_PERIOD_MS 1

// typedef struct {
//     int motor_num[NUMOFMOTOR];
//     float time_stamp;
//     double motor_pos[NUMOFMOTOR];
//     double motor_vel[NUMOFMOTOR];
//     double ctrl_input[NUMOFMOTOR];
// } SharedData;

SharedData *sim_data; 

/* 컨트롤러 */
int shm_fd;
void *shm_ptr;


using namespace std;
//using namespace Eigen;

std::mutex mu;
auto MCL_model = std::make_shared<MCL::MCLmodel_>();

char filename[] = "/home/mcl/robot_ws_chad/src/mujoco_example/pendulum.xml";

// MuJoCo data structures
mjModel* m = NULL;                  // MuJoCo model
mjData* d = NULL;                   // MuJoCo data
mjvCamera cam;                      // abstract camera
mjvOption opt;                      // visualization options
mjvScene scn;                       // abstract scene
mjrContext con;                     // custom GPU context



int part = 0;
double cnt =0;
// mouse interaction
bool button_left = false;
bool button_middle = false;
bool button_right =  false;
double lastx = 0;
double lasty = 0;

// holders of one step history of time and position to calculate dertivatives
mjtNum position_history = 0;
mjtNum previous_time = 0;

// controller related variables
float_t ctrl_update_freq = 100;
mjtNum last_update = 0.0;
mjtNum ctrl;


// keyboard callback
void keyboard(GLFWwindow* window, int key, int scancode, int act, int mods)
{
    // backspace: reset simulation
    if( act==GLFW_PRESS && key==GLFW_KEY_BACKSPACE )
    {
        mj_resetData(m, d);
        mj_forward(m, d);
    }
}


// mouse button callback
void mouse_button(GLFWwindow* window, int button, int act, int mods)
{
    // update button state
    button_left =   (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS);
    button_middle = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS);
    button_right =  (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS);

    // update mouse position
    glfwGetCursorPos(window, &lastx, &lasty);
}



// mouse move callback
void mouse_move(GLFWwindow* window, double xpos, double ypos)
{
    // no buttons down: nothing to do
    if( !button_left && !button_middle && !button_right )
        return;

    // compute mouse displacement, save
    double dx = xpos - lastx;
    double dy = ypos - lasty;
    lastx = xpos;
    lasty = ypos;

    // get current window size
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // get shift key state
    bool mod_shift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS);

    // determine action based on mouse button
    mjtMouse action;
    if( button_right )
        action = mod_shift ? mjMOUSE_MOVE_H : mjMOUSE_MOVE_V;
    else if( button_left )
        action = mod_shift ? mjMOUSE_ROTATE_H : mjMOUSE_ROTATE_V;
    else
        action = mjMOUSE_ZOOM;

    // move camera
    mjv_moveCamera(m, action, dx/height, dy/height, &scn, &cam);
}


// scroll callback
void scroll(GLFWwindow* window, double xoffset, double yoffset)
{
    
    // emulate vertical mouse motion = 5% of window height
    mjv_moveCamera(m, mjMOUSE_ZOOM, 0, -0.05*yoffset, &scn, &cam);
}


long t1 = 0;
long ts = 0;
long old_t1= 0;
long delta_t1= 0;
double sampling_ms= 0;
uint64_t ticks;
uint32_t overrun = 0;
int tfd; // desired timer
struct timespec trt; // 실제 타이머
struct itimerspec timer_conf; //
struct timespec expected; //

void apply_ctrl(MuJoCoMessageHandler *msg_handler,timespec trt,int err) // 1khz로 돌아감.
{
    clock_gettime(CLOCK_MONOTONIC, &trt); //get the system time

    old_t1 = t1;
    t1 = trt.tv_nsec;
    ts = trt.tv_sec;
    delta_t1 = t1 - old_t1;
    sampling_ms = (double)delta_t1 * 0.000001;
    double jitter = sampling_ms - 1.000; 

    // cout << "sampling = "<< sampling_ms <<endl;
    // sim_data ->ctrl_input[0] = 0;
    cnt += 0.001;
    msg_handler->joint_callback_shm(sim_data,d);
    
    
    // memcpy(shm_ptr, sim_data, sizeof(SharedData));

    // msg_handler->joint_callback();
    mjtNum loop_time = d->time - previous_time;
    previous_time = d->time;

    // printf("looptime CPU = %.4f, simulation time = %.4f\n", sampling_ms, loop_time);

    msg_handler->actuator_cmd_callback_shm(sim_data,d);
    
    err = read(tfd, &ticks,sizeof(ticks));  //해당 타이머가 설정한 간격으로 발생한 타이머 틱수를 읽어옴.
        if(err<0) error(1,errno, "read()");

    // std::cout <<"input = " << sim_data->ctrl_input[0] <<std::endl;
    // d->ctrl[0] =
}

void PhysicsThread(GLFWwindow* window, MuJoCoMessageHandler *msg_handler) 
{ 
    
    clock_gettime(CLOCK_MONOTONIC, &expected); 
    tfd = timerfd_create(CLOCK_MONOTONIC, 0); //create timer descriptor
    if(tfd == -1) error(1, errno, "timerfd_create()");

    //타이머 configuration 설정
    timer_conf.it_value = expected; //from now
    timer_conf.it_interval.tv_sec = 0;
    timer_conf.it_interval.tv_nsec = RT_PERIOD_MS*1000000; //interval with RT_PERIOD_MS

    // 타이머 세팅 instance에 적용
    int err = timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timer_conf, NULL); //set the timer descriptor
    if(err) error(1, errno, "timerfd_setting()");
        

    sim_data -> ctrl_input[0] = 0;
    while( !glfwWindowShouldClose(window) && cnt <10)
    // while(cnt <10)
    {
        // advance interactive simulation for 1/60 sec
        //  Assuming MuJoCo can simulate faster than real-time, which it usually can,
        //  this loop will finish on time for the next frame to be rendered at 60 fps.
        //  Otherwise add a cpu timer and exit this loop when it is time to render.
        mjtNum simstart = d->time;
        while( d->time - simstart < 1.0/60.0 )
        {
            // std::lock_guard<std::mutex> lock(mu);
            mj_step(m, d);
            apply_ctrl(msg_handler,trt,err);
            // cout << cnt <<endl;
            
        }

        // get framebuffer viewport
        mjrRect viewport = {0, 0, 0, 0};
        glfwGetFramebufferSize(window, &viewport.width, &viewport.height);

            // update scene and render
        mjv_updateScene(m, d, &opt, NULL, &cam, mjCAT_ALL, &scn);
        mjr_render(viewport, &scn, &con);
        //printf("{%f, %f, %f, %f, %f, %f};\n",cam.azimuth,cam.elevation, cam.distance,cam.lookat[0],cam.lookat[1],cam.lookat[2]);

        // swap OpenGL buffers (blocking call due to v-sync)
        glfwSwapBuffers(window);

        // process pending GUI events, call GLFW callbacks
        glfwPollEvents();
    }
    rclcpp::shutdown(); 
   
}



// main function
int main(int argc, const char** argv)
{
    
    rclcpp::init(argc, argv);
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
  if (shm_fd == -1) {
      perror("shm_open");
      exit(EXIT_FAILURE);
  }
  // 공유 메모리 매핑
  shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
      perror("mmap");
      exit(EXIT_FAILURE);
  }
  
    sim_data = (SharedData *)shm_ptr;
    
    // 공유 메모리에서 구조체 읽기

    std::cout << " MuJoCo start " <<std::endl;

    // load and compile model
    char error[1000] = "Could not load binary model";

    // check command-line arguments
    if( argc<2 )
        m = mj_loadXML(filename, 0, error, 1000);

    else
        if( strlen(argv[1])>4 && !strcmp(argv[1]+strlen(argv[1])-4, ".mjb") )
            m = mj_loadModel(argv[1], 0);
        else
            m = mj_loadXML(argv[1], 0, error, 1000);
    if( !m )
        mju_error_s("Load model error: %s", error);

    // make data
    d = mj_makeData(m);


    // init GLFW
    if( !glfwInit() )
        mju_error("Could not initialize GLFW");

    // create window, make OpenGL context current, request v-sync
    GLFWwindow* window = glfwCreateWindow(1244, 700, "Demo", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // initialize visualization data structures
    mjv_defaultCamera(&cam);
    mjv_defaultOption(&opt);
    mjv_defaultScene(&scn);
    mjr_defaultContext(&con);
    mjv_makeScene(m, &scn, 2000);                // space for 2000 objects
    mjr_makeContext(m, &con, mjFONTSCALE_150);   // model-specific context

    // install GLFW mouse and keyboard callbacks
    glfwSetKeyCallback(window, keyboard);
    glfwSetCursorPosCallback(window, mouse_move);
    glfwSetMouseButtonCallback(window, mouse_button);
    glfwSetScrollCallback(window, scroll);

    double arr_view[] = {89.608063, -11.588379, 5, 0.000000, 0.000000, 0.000000};
    cam.azimuth = arr_view[0];
    cam.elevation = arr_view[1];
    cam.distance = arr_view[2];
    cam.lookat[0] = arr_view[3];
    cam.lookat[1] = arr_view[4];
    cam.lookat[2] = arr_view[5];

    // use the first while condition if you want to simulate for a period.
    // std::thread physicsthreadhandle(&PhysicsThread,window);
    // m->opt.timestep = 0.001;
    auto message_handle = std::make_shared<MuJoCoMessageHandler>(MCL_model.get());
    auto spin_func = [](std::shared_ptr<MuJoCoMessageHandler> node_ptr) 
    {
        rclcpp::spin(node_ptr);
    };
    std::thread spin_thread(spin_func, message_handle);

    PhysicsThread(window, message_handle.get());
    // free visualization storage
    mjv_freeScene(&scn);
    mjr_freeContext(&con);

    // free MuJoCo model and data, deactivate
    mj_deleteData(d);
    mj_deleteModel(m);

    // terminate GLFW (crashes with Linux NVidia drivers)
    #if defined(__APPLE__) || defined(_WIN32)
        glfwTerminate();
    #endif

    // pthread_cancel(spin_thread);
    spin_thread.join();
       
    return 1;
}
