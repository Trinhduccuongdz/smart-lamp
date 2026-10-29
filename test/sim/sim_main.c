/** @file sim_main.c  @brief Điểm vào của bản mô phỏng: giống main.c, thêm task kịch bản Sim. */
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "app_ipc.h"
#include "task_input.h"
#include "task_lamp.h"
#include "task_display.h"
#include "task_console.h"
#include "task_log.h"
#include "task_monitor.h"
void sim_start(void);
int main(void)
{
    board_init(); app_ipc_init();
    task_log_create(); task_console_create(); task_monitor_create();
    task_input_create(); task_lamp_create(); task_display_create();
    sim_start();
    vTaskStartScheduler();
    return 0;
}
