/*! @file
  @brief
  Hardware abstraction layer
        for ESP32

  <pre>
  Copyright (C) 2016-2018 Kyushu Institute of Technology.
  Copyright (C) 2016-2018 Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.
  </pre>
*/

#ifndef MRBC_SRC_HAL_H_
#define MRBC_SRC_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/***** Feature test switches ************************************************/
/***** System headers *******************************************************/
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


/***** Local headers ********************************************************/
/***** Constant values ******************************************************/
/***** Macros ***************************************************************/
#ifndef MRBC_SCHEDULER_EXIT
#define MRBC_SCHEDULER_EXIT 1
#endif


/***** Typedefs *************************************************************/
/***** Global variables *****************************************************/
/***** Function prototypes **************************************************/
void mrbc_tick(void);

#ifndef MRBC_NO_TIMER
void hal_init(void);
void hal_enable_irq(void);
void hal_disable_irq(void);
# define hal_idle_cpu()    float tickUnit = 1/portTICK_PERIOD_MS;vTaskDelay(tickUnit < 1 ? 1 : tickUnit)

#else // MRBC_NO_TIMER
# define hal_init()        ((void)0)
# define hal_enable_irq()  ((void)0)
# define hal_disable_irq() ((void)0)
# define hal_idle_cpu()    (vTaskDelay(1/portTICK_PERIOD_MS), mrbc_tick())

#endif


/***** Inline functions *****************************************************/

//================================================================
/*!@brief
  Write

  @param  fd    dummy, but 1.
  @param  buf   pointer of buffer.
  @param  nbytes        output byte length.
*/
inline static int hal_write(int fd, const void *buf, int nbytes)
{
  return write(1, buf, nbytes);
}

//================================================================
/*!@brief
  Flush write baffer

  @param  fd    dummy, but 1.
*/
inline static int hal_flush(int fd)
{
  return fsync(1);
}


#ifdef __cplusplus
}
#endif
#endif // ifndef MRBC_HAL_H_
