#ifndef CAMERA_BOARD_CONFIG_H
#define CAMERA_BOARD_CONFIG_H

/*
 * User-adjustable recording timing.
 *
 * The camera's automatic recording is always stopped 5 seconds after power-on.
 * These two values are measured from the commands immediately before them.
 * 1000 milliseconds = 1 second.
 */
#define CAMERA_BOARD_START_DELAY_AFTER_AUTO_STOP_MS     15000U
#define CAMERA_BOARD_RECORD_DURATION_MS                 10000U

/*
 * The stop command starts the SD-card save immediately. This guard time only
 * keeps camera power stable while the camera finishes writing the file.
 */
#define CAMERA_BOARD_SD_SAVE_DELAY_MS                   10000U

/* Camera UART  */
#define CAMERA_BOARD_CAMERA_UART_BAUD_RATE         115200U
#define CAMERA_BOARD_CAMERA_UART_TX_TIMEOUT_MS     50U

/* Debug/output UART  */
#define CAMERA_BOARD_OUTPUT_UART_TX_TIMEOUT_MS     50U

/* Power behavior: set to 1U to turn the cameras off after the SD save delay. */
#define CAMERA_BOARD_POWER_OFF_AFTER_SAVE          0U

/* Catch invalid timing changes at build time. */
#if CAMERA_BOARD_RECORD_DURATION_MS == 0U
#error "Recording duration must be greater than zero"
#endif

#endif /* CAMERA_BOARD_CONFIG_H */
