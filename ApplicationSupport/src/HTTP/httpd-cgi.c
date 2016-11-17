/**
 * \addtogroup httpd
 * @{
 */

/**
 * \file
 *         Web server script interface
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright (c) 2001-2006, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: httpd-cgi.c,v 1.2 2006/06/11 21:46:37 adam Exp $
 *
 */

#include "uip.h"
#include "psock.h"
#include "httpd.h"
#include "httpd-cgi.h"
#include "httpd-fs.h"

#include <relay.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dali_click.h>
#include <types.h>

HTTPD_CGI_CALL(file, "file-stats", file_stats);
HTTPD_CGI_CALL(tcp, "tcp-connections", tcp_stats);
HTTPD_CGI_CALL(net, "net-stats", net_stats);
HTTPD_CGI_CALL(rtos, "rtos-stats", rtos_stats );
HTTPD_CGI_CALL(run, "run-time", run_time );
HTTPD_CGI_CALL(io, "relay-io", relay_io );
HTTPD_CGI_CALL(sensor, "sensor_data", sensor_data);
HTTPD_CGI_CALL(dali, "dali_io", dali_io);
HTTPD_CGI_CALL(relay, "relay_click", relay_click);
HTTPD_CGI_CALL(slaves, "slaves_data", slaves_data);
HTTPD_CGI_CALL(clickboardconfig, "clickboard_config", clickboard_config);
HTTPD_CGI_CALL(cloud, "cloud_config", cloud_config);
HTTPD_CGI_CALL(relayr, "relayr_config", relayr_config);


static const struct httpd_cgi_call *calls[] = { &file, &tcp, &net, &rtos, &run, &io, &sensor, &dali, &relay, &slaves, &clickboardconfig, &cloud, &relayr, NULL };

extern config_t *gpconfig;

/*---------------------------------------------------------------------------*/
static
PT_THREAD(nullfunction(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_cgifunction
httpd_cgi(char *name)
{
  const struct httpd_cgi_call **f;

  /* Find the matching name in the table, return the function. */
  for(f = calls; *f != NULL; ++f) {
    if(strncmp((*f)->name, name, strlen((*f)->name)) == 0) {
      return (*f)->function;
    }
  }
  return nullfunction;
}
/*---------------------------------------------------------------------------*/
static unsigned short
generate_file_stats(void *arg)
{
  char *f = (char *)arg;
  return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE, "%5u", httpd_fs_count(f));
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(file_stats(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, generate_file_stats, strchr(ptr, ' ') + 1);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static const char closed[] =   /*  "CLOSED",*/
{0x43, 0x4c, 0x4f, 0x53, 0x45, 0x44, 0};
static const char syn_rcvd[] = /*  "SYN-RCVD",*/
{0x53, 0x59, 0x4e, 0x2d, 0x52, 0x43, 0x56,
 0x44,  0};
static const char syn_sent[] = /*  "SYN-SENT",*/
{0x53, 0x59, 0x4e, 0x2d, 0x53, 0x45, 0x4e,
 0x54,  0};
static const char established[] = /*  "ESTABLISHED",*/
{0x45, 0x53, 0x54, 0x41, 0x42, 0x4c, 0x49, 0x53, 0x48,
 0x45, 0x44, 0};
static const char fin_wait_1[] = /*  "FIN-WAIT-1",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
 0x54, 0x2d, 0x31, 0};
static const char fin_wait_2[] = /*  "FIN-WAIT-2",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
 0x54, 0x2d, 0x32, 0};
static const char closing[] = /*  "CLOSING",*/
{0x43, 0x4c, 0x4f, 0x53, 0x49,
 0x4e, 0x47, 0};
static const char time_wait[] = /*  "TIME-WAIT,"*/
{0x54, 0x49, 0x4d, 0x45, 0x2d, 0x57, 0x41,
 0x49, 0x54, 0};
static const char last_ack[] = /*  "LAST-ACK"*/
{0x4c, 0x41, 0x53, 0x54, 0x2d, 0x41, 0x43,
 0x4b, 0};

static const char *states[] = {
  closed,
  syn_rcvd,
  syn_sent,
  established,
  fin_wait_1,
  fin_wait_2,
  closing,
  time_wait,
  last_ack};


static unsigned short
generate_tcp_stats(void *arg)
{
  struct uip_conn *conn;
  struct httpd_state *s = (struct httpd_state *)arg;

  conn = &uip_conns[s->count];
  return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE,
		 "<tr><td>%d</td><td>%u.%u.%u.%u:%u</td><td>%s</td><td>%u</td><td>%u</td><td>%c %c</td></tr>\r\n",
		 htons(conn->lport),
		 htons(conn->ripaddr[0]) >> 8,
		 htons(conn->ripaddr[0]) & 0xff,
		 htons(conn->ripaddr[1]) >> 8,
		 htons(conn->ripaddr[1]) & 0xff,
		 htons(conn->rport),
		 states[conn->tcpstateflags & UIP_TS_MASK],
		 conn->nrtx,
		 conn->timer,
		 (uip_outstanding(conn))? '*':' ',
		 (uip_stopped(conn))? '!':' ');
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(tcp_stats(struct httpd_state *s, char *ptr))
{

  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  for(s->count = 0; s->count < UIP_CONNS; ++s->count) {
    if((uip_conns[s->count].tcpstateflags & UIP_TS_MASK) != UIP_CLOSED) {
      PSOCK_GENERATOR_SEND(&s->sout, generate_tcp_stats, s);
    }
  }

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static unsigned short
generate_net_stats(void *arg)
{
  struct httpd_state *s = (struct httpd_state *)arg;
  return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE,
		  "%5u\n", ((uip_stats_t *)&uip_stat)[s->count]);
}

static
PT_THREAD(net_stats(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  ( void ) ptr;
#if UIP_STATISTICS

  for(s->count = 0; s->count < sizeof(uip_stat) / sizeof(uip_stats_t);
      ++s->count) {
    PSOCK_GENERATOR_SEND(&s->sout, generate_net_stats, s);
  }

#endif /* UIP_STATISTICS */

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/

extern void vTaskList( char *pcWriteBuffer );
//static char cCountBuf[ 128 ];
static char cCountBuf[700];
long lRefreshCount = 0;
static unsigned short
generate_rtos_stats(void *arg)
{
	( void ) arg;
	lRefreshCount++;
	sprintf( cCountBuf, "<p><br>Refresh count = %d<p><br>", (int)lRefreshCount);
    vTaskList( uip_appdata );
	strcat( uip_appdata, cCountBuf );

	return strlen( uip_appdata );
}
/*---------------------------------------------------------------------------*/


static
PT_THREAD(rtos_stats(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_rtos_stats, NULL);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/

char *pcStatus;
unsigned long ulString;

static unsigned short generate_io_state( void *arg )
{
	( void ) arg;
	char * pcValue;
	if( getRelayStatus() == OFF )
	{
		pcStatus = "";
		pcValue = "0";
	}
	else
	{
		pcStatus = "checked";
		pcValue = "1";
	}

	sprintf( uip_appdata,
		"<input type=\"checkbox\" name=\"RELAY0\" value=\"1\" %s>RELAY<p><p> <input type=\"hidden\" name=\"PREVIOUS_VALUE\" value=\"%s\" checked><p><p>", pcStatus, pcValue );

	return strlen( uip_appdata );
}
/*---------------------------------------------------------------------------*/

extern void vTaskGetRunTimeStats( char *pcWriteBuffer );
static unsigned short
generate_runtime_stats(void *arg)
{
	( void ) arg;
	lRefreshCount++;
	sprintf( cCountBuf, "<p><br>Refresh count = %d", (int)lRefreshCount );
    vTaskGetRunTimeStats( uip_appdata );
	strcat( uip_appdata, cCountBuf );

	return strlen( uip_appdata );
}
/*---------------------------------------------------------------------------*/


static
PT_THREAD(run_time(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_runtime_stats, NULL);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/


static PT_THREAD(relay_io(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_io_state, NULL);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/

extern float getTemperature();
extern float getHumidity();
extern int getmW();

static unsigned short generate_sensor_data(void *arg) {

	/*convert float to two integers*/
	(void) arg;
	float temp = getTemperature();
	int d1 = temp;
	float temp2 = temp - d1;
	int d2 = (int)(temp2  * 10);
	char mW[4];
	itoa(getmW(),mW,10);
	//int mW = getmW();

	/*Display Sensor Data*/
	sprintf(cCountBuf, " ");
	if (gethdc1000_isactive()) {
		sprintf(cCountBuf, "%i.%i<br>%i", d1, d2, (int)getHumidity());
		if(getuv_isactive()) {
			strcat(cCountBuf, "<br>");
		}
	} else {
		sprintf(cCountBuf, " - <br> - ");
		if(getuv_isactive()) {
			strcat(cCountBuf, "<br>");
		}
	}
	if (getuv_isactive()) {
		strcat(cCountBuf, mW);
	} else {
		strcat(cCountBuf, "<br> - ");
	}

	strcat( uip_appdata, cCountBuf);

	return strlen(uip_appdata);

}


static PT_THREAD(sensor_data(struct httpd_state *s, char *ptr)) {
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_sensor_data, NULL);
  PSOCK_END(&s->sout);
}

extern bool getwaitForAnswer();
extern int getBackwardStatus();
extern int getBackwardFrame();

static unsigned short generate_dali_io(void *arg) {

	//get answer from dali_click.c
	if (getwaitForAnswer()) {

		switch (getBackwardStatus()) {
			case 0:
				sprintf(cCountBuf, "ANSWER_NOT_AVAILABLE");
				break;
			case 1:
				sprintf(cCountBuf, "ANSWER_NOTHING_RECEIVED");
				break;
			case 2:
				sprintf(cCountBuf, "Answer: %02x", getBackwardFrame());
				break;
			case 3:
				sprintf(cCountBuf, "ANSWER_INVALID_DATA\r\nAnswer: %i", getBackwardFrame());
			break;
			case 4:
				sprintf(cCountBuf, "ANSWER_TOO_EARLY");
				break;
			default:
				sprintf(cCountBuf, "ERROR");
				break;
		}

	} else {
		sprintf(cCountBuf, "No answer expected!");
	}

	strcat (uip_appdata, cCountBuf);

	return strlen(uip_appdata);

}

static PT_THREAD(dali_io(struct httpd_state *s, char *ptr)) {
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_dali_io, NULL);
  PSOCK_END(&s->sout);
}

static unsigned short generate_relay_click(void *arg) {


	if(getrelay_isactive()) {
	sprintf(cCountBuf, "<form><table><tr><td>Relay 1</td><td><form> <button type=\"submit\" name=\"action\" value=\"Relay1on\">ON</button></td><td><button type=\"submit\" name=\"action\" value=\"Relay1off\">OFF</button>"
	                    "</td></tr><tr><td>Relay 2</td><td><button type=\"submit\" name=\"action\" value=\"Relay2on\">ON</button></td><td><button type=\"submit\" name=\"action\" value=\"Relay2off\">OFF</button></form></td></tr></table>");
	} else {
		sprintf(cCountBuf, "Relay Clickboard not configured !");
	}

	strcat (uip_appdata, cCountBuf);

	return strlen(uip_appdata);

}

static PT_THREAD(relay_click(struct httpd_state *s, char *ptr)) {
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_relay_click, NULL);
  PSOCK_END(&s->sout);
}

extern int getSlaves();

static unsigned short generate_slaves_data(void *arg) {

	int slaves = (int)getSlaves();

	sprintf(cCountBuf, "Found %i Slaves on BUS. Address from 1 to %i", slaves);

	strcat (uip_appdata, cCountBuf);

	return strlen(uip_appdata);

}

static PT_THREAD(slaves_data(struct httpd_state *s, char *ptr)) {
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_slaves_data, NULL);
  PSOCK_END(&s->sout);
}

static unsigned short generate_clickboard_config(void *arg) {

    sprintf(cCountBuf, "");

//    strcat(cCountBuf, "<tr><td>HDC1000</td><td><input type=\"radio\" name=\"M1\" value=\"hdc1000\"");
//    if(gpconfig->M1 == 1) strcat(cCountBuf, " checked");
//    strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"hdc1000\"");
//    if(gpconfig->M2 == 1) strcat(cCountBuf, " checked");
//    strcat(cCountBuf, "></td></tr>");

	strcat(cCountBuf, "<tr><td>Relay</td><td><input type=\"radio\" name=\"M1\" value=\"relay\"");
	if(gpconfig->M1 == 2) strcat(cCountBuf, " checked");
	strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"relay\"");
	if(gpconfig->M2 == 2) strcat(cCountBuf, " checked");
	strcat(cCountBuf, "></td></tr>");

	strcat(cCountBuf, "<tr><td>DALI</td><td><input type=\"radio\" name=\"M1\" value=\"dali\"");
	if(gpconfig->M1 == 3) strcat(cCountBuf, " checked");
	strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"dali\"");
	if(gpconfig->M2 == 3) strcat(cCountBuf, " checked");
	strcat(cCountBuf, "></td></tr>");

//	strcat(cCountBuf, "<tr><td>Color</td><td><input type=\"radio\" name=\"M1\" value=\"color\"");
//	if(gpconfig->M1 == 4) strcat(cCountBuf, " checked");
//	strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"color\"");
//	if(gpconfig->M2 == 4) strcat(cCountBuf, " checked");
//	strcat(cCountBuf, "></td></tr>");

	strcat(cCountBuf, "<tr><td>UV</td><td><input type=\"radio\" name=\"M1\" value=\"uv\"");
	if(gpconfig->M1 == 5) strcat(cCountBuf, " checked");
	strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"uv\" disabled></td></tr>");

    strcat(cCountBuf, "<tr><td>Thermo3</td><td><input type=\"radio\" name=\"M1\" value=\"thermo3\"");
    if(gpconfig->M1 == 6) strcat(cCountBuf, " checked");
    strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"thermo3\"");
    if(gpconfig->M2 == 6) strcat(cCountBuf, " checked");
    strcat(cCountBuf, "></td></tr>");

    strcat(cCountBuf, "<tr><td>LCD</td><td><input type=\"radio\" name=\"M1\" value=\"lcd\"");
    if(gpconfig->M1 == 7) strcat(cCountBuf, " checked");
    strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"lcd\"");
    if(gpconfig->M2 == 7) strcat(cCountBuf, " checked");
    strcat(cCountBuf, "></td></tr>");

	strcat(cCountBuf, "<tr><td>None</td><td><input type=\"radio\" name=\"M1\" value=\"none\"");
	if(gpconfig->M1 == 0) strcat(cCountBuf, " checked");
	strcat(cCountBuf, "></td><td><input type=\"radio\" name=\"M2\" value=\"none1\"");
	if(gpconfig->M2 == 0) strcat(cCountBuf, " checked");
    strcat(cCountBuf, ">");

	strcat (uip_appdata, cCountBuf);

	return strlen(uip_appdata);

}

static PT_THREAD(clickboard_config(struct httpd_state *s, char *ptr)) {
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_clickboard_config, NULL);
  PSOCK_END(&s->sout);
}

static unsigned short generate_cloud_config(void *arg) {

    if (gpconfig->active == 1) {
        sprintf(cCountBuf,
                " checked>No<input type=\"radio\" name=\"activate_mqtt\" value=\"no\"><br><br><input type=\"submit\" value=\"Save\" width=\"100px\"></form><br><a href=\"relayrip.shtml\">IP Address</a> | <a href=\"relayruser.shtml\">User</a> | <a href=\"relayrpass.shtml\">Password</a> | <a href=\"relayrid.shtml\">ClientID</a> | <a href=\"relayrtopic.shtml\">Topic</a><br><br>");

    } else {
        sprintf(cCountBuf, ">No<input type=\"radio\" name=\"activate_mqtt\" value=\"no\" checked><br><br><input type=\"submit\" value=\"Save\" width=\"100px\"></form>");
    }

    strcat (uip_appdata, cCountBuf);

    return strlen(uip_appdata);

}

static PT_THREAD(cloud_config(struct httpd_state *s, char *ptr)) {
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_cloud_config, NULL);
  PSOCK_END(&s->sout);
}

static unsigned short generate_relayr_config(void *arg) {

    sprintf(cCountBuf, "Host:     ");
    strcat(cCountBuf, gpconfig->hostname);
    strcat(cCountBuf, "<br>Username: ");
    strcat(cCountBuf, gpconfig->user);
    strcat(cCountBuf, "<br>Password: ");
    strcat(cCountBuf, gpconfig->password);
    strcat(cCountBuf, "<br>ClientID: ");
    strcat(cCountBuf, gpconfig->clientid);
    strcat(cCountBuf, "<br>Topic:    ");
    strcat(cCountBuf, gpconfig->topic);

    strcat (uip_appdata, cCountBuf);

    return strlen(uip_appdata);

}

static PT_THREAD(relayr_config(struct httpd_state *s, char *ptr)) {
  PSOCK_BEGIN(&s->sout);
  ( void ) ptr;
  PSOCK_GENERATOR_SEND(&s->sout, generate_relayr_config, NULL);
  PSOCK_END(&s->sout);
}



/** @} */
