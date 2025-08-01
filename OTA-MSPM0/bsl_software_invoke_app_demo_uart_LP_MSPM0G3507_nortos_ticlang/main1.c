// #include <ti_msp_dl_config.h>

// int main(void)
// {
//     // Initialize the system configuration
//     SYSCFG_DL_init();
    
//     // Turn on the LED (using the same LED that's used in the existing code)
//     // Clear the pin to turn on the LED (assuming active-low)
//     DL_GPIO_clearPins(GPIO_LED_PA0_PORT, GPIO_LED_PA0_PIN);
    
//     // Alternative: If the LED is active-high, use this instead:
//     // DL_GPIO_setPins(GPIO_LED_PA0_PORT, GPIO_LED_PA0_PIN);
    
//     // Keep the program running
//     while (1) {
//         // Infinite loop to keep the LED on
//         // You can add other functionality here if needed
//     }
// }