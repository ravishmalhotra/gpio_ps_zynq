/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * ledtoggle.c: simple test application for the Zybo board MIO pins
 * and the interrupt system.
 *
 * The application toggles the LED (MIO 7) on every press of the
 * pushbutton BTN4
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xgpiops.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xscugic.h"
#include "xil_exception.h"

#define LED_PIN 7
#define SW_PIN 50


#define GPIO_DEVICE_ID XPAR_PS7_GPIO_0_DEVICE_ID
#define GIC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#define GPIO_INTERRUPT_ID XPS_GPIO_INT_ID


//Global variables for keeping track of toggle

u8 toggle=0;


//Function prototypes
static void led_toggle (void *CallBackRef, int Bank, u32 Status);


// Main
int main()
{

	int gpio_init_status;
	int interrupt_init_status;


	XGpioPs_Config *gpio_config;
	XGpioPs gpio_inst;


	XScuGic_Config *gic_config;
    Xil_ExceptionInit();

	XScuGic gic_inst;

	init_platform();

	 // Initialize GPIO
	gpio_config=XGpioPs_LookupConfig(GPIO_DEVICE_ID);


	//Initialise GPIO instant
	gpio_init_status=XGpioPs_CfgInitialize(&gpio_inst, gpio_config,gpio_config->BaseAddr);

    if (gpio_init_status != XST_SUCCESS) {
        xil_printf("GPIO could not be configured appropriately");
    }




    //Set LED_PIN as output
    XGpioPs_SetDirectionPin(&gpio_inst, LED_PIN, 1);
    XGpioPs_SetOutputEnablePin(&gpio_inst,LED_PIN, 1);
    XGpioPs_WritePin(&gpio_inst, LED_PIN, 0);


    //Set SW_PIN as input
   XGpioPs_SetDirectionPin(&gpio_inst, SW_PIN, 0);



    gic_config= XScuGic_LookupConfig(GIC_DEVICE_ID);
    interrupt_init_status= XScuGic_CfgInitialize(&gic_inst, gic_config,gic_config->CpuBaseAddress);


    if (interrupt_init_status != XST_SUCCESS)
      {
          xil_printf("GPIO could not be configured appropriately");
      }


   //Initialize Interrupts for gpio


	//Connect the GIC to the exception system
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_InterruptHandler)XScuGic_InterruptHandler,(void *)&gic_inst);


	//Connect the GPIO interrupt to the GIC
	XScuGic_Connect(&gic_inst, GPIO_INTERRUPT_ID,(Xil_InterruptHandler)XGpioPs_IntrHandler, (void *) &gpio_inst);


	//Define Interrupt source as rising edge on SW pin MIO51
    XGpioPs_SetIntrTypePin(&gpio_inst, SW_PIN,  XGPIOPS_IRQ_TYPE_EDGE_RISING);

	//Set the GPIO callback function
	XGpioPs_SetCallbackHandler(&gpio_inst, (void *)&gpio_inst, led_toggle);


  	//Find registers with Interrupt flag high. Acknowledge those interrupts

	XGpioPs_IntrClearPin(&gpio_inst, SW_PIN);
   	XGpioPs_IntrEnablePin(&gpio_inst, SW_PIN);



     //Enable the GPIO interrup
   	XScuGic_Enable(&gic_inst, GPIO_INTERRUPT_ID);
   	printf("GIC enabled \n\r");


   	//Enable Global interrupts
    Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
    printf("Global Interrupt Set Up\n\r");

   while (1) {


   }

    return 0;
}


//LED blink function

static void led_toggle (void *CallBackRef, int Bank, u32 Status) {


	XGpioPs *gpio_inst = (XGpioPs *)CallBackRef;

	xil_printf("GpioPs Handler running… Bank: %d | Status: %x \r\n", Bank,Status);

	toggle = !toggle;
	XGpioPs_WritePin(gpio_inst, LED_PIN,toggle);


}


