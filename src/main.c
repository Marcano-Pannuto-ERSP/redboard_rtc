// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <rtc.h>

#include <cli.h>
#include <uart.h>
#include <syscalls.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <sys/time.h>

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

struct uart uart;
struct am1815 rtc;
struct spi_bus spi;
struct spi_device rtc_spi;
struct cli cli;

__attribute__((constructor))
static void redboard_init(void)
{
	// Prepare MCU by init-ing clock, cache, and power level operation
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();
	am_hal_sysctrl_fpu_enable();
	am_hal_sysctrl_fpu_stacking_enable(true);

	spi_bus_init(&spi, 0);
	spi_bus_enable(&spi);
	spi_bus_init_device(&spi, &rtc_spi, SPI_CS_3, 2000000u);
	am1815_init(&rtc, &rtc_spi);

	cli_init(&cli);
	uart_init(&uart, UART_INST0);
	syscalls_uart_init(&uart);
	syscalls_rtc_init(&rtc);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();
}

__attribute__((destructor))
static void redboard_shutdown(void)
{
	// Any destructors/code that should run when main returns should go here
	cli_destroy(&cli);
}

struct command
{
	const char *command;
	const char *help;
	void *context;
	int (*function)(void *context, const char *line);
};

#define ARRAY_SIZE(ARG) (sizeof(ARG)/sizeof(*ARG))

int command_exit(void *context, const char *line)
{
	(void)context;
	(void)line;
	return -2;
}
int command_name(void *context, const char *line)
{
	(void)context;
	(void)line;
	printf("Redboard Artemis RTC Configuration\r\n");
	return 0;
}

int command_help(void *context, const char *line);
int command_echo(void *context, const char *line)
{
	(void)line;
	struct cli *cli = context;
	cli->echo = !cli->echo;
	return 0;
}

int command_history(void *context, const char *line)
{
	(void)context;
	(void)line;
	size_t max = ring_buffer_in_use(&cli.history);
	for (size_t i = 0; i < max; ++i)
	{
		printf("%d %s\r\n", i+1, (char*)ring_buffer_get(&cli.history, max-1-i));
	}
	return 0;
}

int command_read(void *context, const char *line)
{
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);

	struct am1815 *rtc = context;
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no address provided\r\n");
		goto err;
	}
	char *ptr;
	long addr = strtol(tok, &ptr, 0);
	if (tok == ptr || addr < 0 || addr > 255)
	{
		printf("Error: invalid address\r\n");
		goto err;
	}
	uint8_t result = am1815_read_register(rtc, addr);
	printf("%"PRIu8"\r\n", result);

	free(buf);
	return 0;

err:
	free(buf);
	return -1;
}

int command_read_bulk(void *context, const char *line)
{
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);

	struct am1815 *rtc = context;
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no address provided\r\n");
		goto err;
	}
	char *ptr;
	long addr = strtol(tok, &ptr, 0);
	if (tok == ptr || addr < 0 || addr > 255)
	{
		printf("Error: invalid address\r\n");
		goto err;
	}

	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no size provided\r\n");
		goto err;
	}
	long size = strtol(tok, &ptr, 0);
	if (tok == ptr || size < 1 || size > 255)
	{
		printf("Error: invalid size\r\n");
		goto err;
	}
	uint8_t *buffer = malloc(size);
	am1815_read_bulk(rtc, addr, buffer, size);
	printf("0x%"PRIX8, buffer[0]);
	for (size_t i = 1; i < (uint8_t)size; ++i)
	{
		printf(" 0x%"PRIX8, buffer[i]);
	}
	printf("\r\n");
	free(buffer);


	free(buf);
	return 0;

err:
	free(buf);
	return -1;
}

int command_write(void *context, const char *line)
{
	struct am1815 *rtc = context;
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no address provided\r\n");
		goto err;
	}
	char *ptr;
	long addr = strtol(tok, &ptr, 0);
	if (tok == ptr || addr < 0 || addr > 255)
	{
		printf("Error: invalid address\r\n");
		goto err;
	}

	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no data provided\r\n");
		goto err;
	}
	long data = strtol(tok, &ptr, 0);
	if (tok == ptr || data < 0 || data > 255)
	{
		printf("Error: invalid data\r\n");
		goto err;
	}

	am1815_write_register(rtc, (uint8_t)addr, (uint8_t)data);
	return 0;

err:
	free(buf);
	return -1;
}

int command_trickle(void *context, const char *line)
{
	struct am1815 *rtc = context;

	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no argument provided\r\n");
		goto err;
	}
	char *ptr;
	long data = strtol(tok, &ptr, 0);
	if (tok == ptr || data < 0 || data > 1)
	{
		printf("Error: invalid data\r\n");
		goto err;
	}

	if (data == 0)
		am1815_disable_trickle(rtc);
	else
		am1815_enable_trickle(rtc);

	return 0;
err:
	free(buf);
	return -1;
}

int command_disable_pin(void *context, const char *line)
{
	(void)line;
	struct am1815 *rtc = context;
	disable_pins(rtc);
	return 0;
}

int command_prog_osc(void *context, const char *line)
{
	(void)line;
	struct am1815 *rtc = context;
	// get access to osillator control register
	am1815_write_register(rtc, 0x1F, 0xA1);

	// clear the OF bit so that a failure isn't detected on start up
	uint8_t OF = am1815_read_register(rtc, 0x1D);
	uint8_t OFmask = 0b00000010;
	uint8_t OFresult = OF & ~OFmask;
	am1815_write_register(rtc, 0x1D, OFresult);

	return 0;
}

int command_osc_failover(void *context, const char *line)
{
	struct am1815 *rtc = context;
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no argument provided\r\n");
		goto err;
	}
	char *ptr;
	long data = strtol(tok, &ptr, 0);
	if (tok == ptr || data < 0 || data > 1)
	{
		printf("Error: invalid data\r\n");
		goto err;
	}
	uint8_t osCtrl = am1815_read_register(rtc, 0x1C);
	uint8_t FOSmask = 0b00001000;
	uint8_t FOSresult;
	if(data == 0)
	{
		// set FOS to 0
		FOSresult = osCtrl & ~FOSmask;
		printf("disabled automatic switching when an oscillator failure is detected\r\n");
	}
	else{
		// set FOs to 1 (default)
		FOSresult = osCtrl | FOSmask;
		printf("enabled automatic switching when an oscillator failure is detected\r\n");
	}
	// get access to oscillator control register
	am1815_write_register(rtc, 0x1F, 0xA1);
	am1815_write_register(rtc, 0x1C, FOSresult);

	return 0;

err:
	free(buf);
	return -1;
}

int command_osc_batover(void *context, const char *line)
{
	struct am1815 *rtc = context;
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no argument provided\r\n");
		goto err;
	}
	char *ptr;
	long data = strtol(tok, &ptr, 0);
	if (tok == ptr || data < 0 || data > 1)
	{
		printf("Error: invalid data\r\n");
		goto err;
	}
	uint8_t osCtrl = am1815_read_register(rtc, 0x1C);
	uint8_t AOSmask = 0b00010000;
	uint8_t AOSresult;
	if(data == 0)
	{
		// set FOS to 0
		AOSresult = osCtrl & ~AOSmask;
		printf("disabled automatic switching when battery powered\r\n");
	}
	else{
		// set FOs to 1 (default)
		AOSresult = osCtrl | AOSmask;
		printf("enabled automatic switching when battery powered\r\n");
	}
	// get access to oscillator control register
	am1815_write_register(rtc, 0x1F, 0xA1);
	am1815_write_register(rtc, 0x1C, AOSresult);

	return 0;
err:
	free(buf);
	return -1;
}

int command_alarm(void *context, const char *line)
{
	struct am1815 *rtc = context;
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no enable argument provided\r\n");
		goto err;
	}
	bool enable;
	if (strcmp(tok, "true") == 0)
		enable = true;
	else if (strcmp(tok, "false") == 0)
		enable = false;
	else
	{
		printf("Error: invalid enable argument\r\n");
		goto err;
	}

	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no pulse argument provided\r\n");
		goto err;
	}
	char *ptr;
	long data = strtol(tok, &ptr, 0);
	if (tok == ptr || data < 0 || data > 3)
	{
		printf("Error: invalid pulse argument\r\n");
		goto err;
	}

	configure_alarm(rtc, enable, (uint8_t)data);

	return 0;

err:
	free(buf);
	return -1;
}

int command_countdown(void *context, const char *line)
{
	struct am1815 *rtc = context;
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);
	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no argument provided\r\n");
		goto err;
	}
	char *ptr;
	double data = strtod(tok, &ptr);

	if (tok == ptr || data < 0 || data > 15360)
	{
		printf("Error: invalid data\r\n");
		goto err;
	}

	configure_countdown(rtc, data);

	return 0;

err:
	free(buf);
	return -1;
}

int command_init(void *context, const char *line)
{
	(void)line;
	struct am1815 *rtc = context;
	// Write to bit 7 of register 1 to signal that this program initialized the RTC
	uint8_t sec = am1815_read_register(rtc, 0x01);
	uint8_t secMask = 0b10000000;
	uint8_t secResult = sec | secMask;
	am1815_write_register(rtc, 0x01, secResult);
	configure_alarm(rtc, true, 1);

	am1815_write_register(rtc, 0x1F, 0x9D);
	am1815_write_register(rtc, 0x30, 0x01);

	return 0;
}

int command_get_time(void *context, const char *line)
{
	(void)line;
	struct am1815 *rtc = context;
	
	struct timeval curr_time = am1815_read_time(rtc);
    am_util_stdio_printf("RTC's current time: %llu seconds, %ld microseconds\r\n", curr_time.tv_sec, curr_time.tv_usec);

	return 0;
}

int command_set_time(void *context, const char *line)
{
	struct am1815 *rtc = context;
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);

	char *tok = strtok(buf, " \t\r\n");
	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no seconds argument provided\r\n");
		goto err;
	}
	char *ptr;
	long seconds = strtol(tok, &ptr, 0);

	tok = strtok(NULL, " \t\r\n");
	if (!tok)
	{
		printf("Error: no hundredths argument provided\r\n");
		goto err;
	}
	long microseconds = strtol(tok, &ptr, 0) * 10000;

	struct timeval tm = {.tv_sec = seconds, .tv_usec = microseconds};
	am1815_write_time(rtc, &tm);

	return 0;

err:
	free(buf);
	return -1;
}

int command_ping(void *context, const char *line)
{
	(void)line;
	struct am1815 *rtc = context;
	
	const unsigned char* request = "request\r\n";
	uart_write(&uart, request, strlen(request));
	struct timeval req_time = am1815_read_time(rtc);

	char response[10];
	uart_read(&uart, response, 10);
	struct timeval resp_time = am1815_read_time(rtc);

	const char to_write[60];
	snprintf("%llu %ld %llu %ld\r\n", to_write, 60, req_time.tv_sec, req_time.tv_usec, resp_time.tv_sec, resp_time.tv_usec);
	uart_write(&uart, to_write, strlen(to_write));

	am_util_stdio_printf("to_write: %s\r\n", to_write);
	
	return 0;
}

struct command commands[] = {
	{ .command = "exit", .help = "Exit this application", .context = NULL, .function = command_exit},
	{ .command = "?", .help = "Check the application name", .context = NULL, .function = command_name},
	{ .command = "history", .help = "Get CLI history", .context = NULL, .function = command_history},
	{ .command = "help", .help = "Get the list of commands, or help for a specific one", .context = NULL, .function = command_help},
	{ .command = "echo", .help = "Toggle console echo", .context = &cli, .function = command_echo},
	{ .command = "read", .help = "Read a register", .context = &rtc, .function = command_read},
	{ .command = "read_bulk", .help = "Read a series of registers", .context = &rtc, .function = command_read_bulk},
	{ .command = "write", .help = "Write to a register", .context = &rtc, .function = command_write},
	{ .command = "trickle", .help = "Control trickle charging", .context = &rtc, .function = command_trickle},
	{ .command = "disable_pin", .help = "Disable default pins", .context = &rtc, .function = command_disable_pin},
	{ .command = "prog_osc", .help = "Default? program oscillator register", .context = &rtc, .function = command_prog_osc},
	{ .command = "osc_failover", .help = "Configure oscillator failover", .context = &rtc, .function = command_osc_failover},
	{ .command = "osc_batover", .help = "Configure oscillator switchover on battery", .context = &rtc, .function = command_osc_batover},
	{ .command = "alarm", .help = "Configure alarm", .context = &rtc, .function = command_alarm},
	{ .command = "countdown", .help = "Configure countdown timer to (0, 15360]s", .context = &rtc, .function = command_countdown},
	{ .command = "init", .help = "Init other stuff???????", .context = &rtc, .function = command_init},
	{ .command = "get_time", .help = "Get RTC's time", .context = &rtc, .function = command_get_time},
	{ .command = "set_time", .help = "Set RTC to a specified time", .context = &rtc, .function = command_set_time},
	// { .command = "change_time", .help = "Change RTC time by an offset", .context = &rtc, .function = command_change_time},
	{ .command = "ping", .help = "Get timestamps of request and response", .context = &rtc, .function = command_ping}, //FIXME what is context here
};

int command_help(void *context, const char *line)
{
	(void)context;
	(void)line;
	for (size_t i = 0; i < ARRAY_SIZE(commands); ++i)
	{
		printf("%s - %s\r\n", commands[i].command, commands[i].help);
	}
	return 0;
}

size_t get_ntokens(const char *str)
{
	char *tmp = malloc(strlen(str) + 1);
	if (!tmp)
		return ((size_t)0)-1; // FIXME what's a sensinble thing if we're out of memory?
	memcpy(tmp, str, strlen(str) + 1);
	size_t count = 0;
	char *tok = strtok(tmp, " \t\r\n");
	if (!tok)
		return 0;
	while (strtok(NULL, " \t\r\n"))
		++count;
	free(tmp);
	return count;
}

int dispatch_command(const char *line)
{
	char *buf = malloc(strlen(line)+1);
	memcpy(buf, line, strlen(line)+1);
	char *tok = strtok(buf, " \t\r\n");
	int result = 0;
	for (size_t i = 0; i < ARRAY_SIZE(commands); ++i)
	{
		const char *command = commands[i].command;
		if (strcmp(command, tok) == 0)
		{
			if (commands[i].function)
				result = commands[i].function(commands[i].context, line);
			break;
		}
	}
	free(buf);
	return result;
}

int main(void)
{
	bool done = false;

	while (!done)
	{
		if (cli.echo)
		{
			printf("> ");
			fflush(stdout);
		}
		cli_line_buffer* buf = cli_read_line(&cli);
		int result = dispatch_command((const char*)buf);

		if (result == -2)
		{
			done = true;
		}
		else if (result == -1)
		{
			printf("Invalid command\r\n");
		}
	}

	printf("Exiting....\r\n");

	return 0;
}
