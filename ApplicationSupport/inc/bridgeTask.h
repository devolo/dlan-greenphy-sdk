/*
 * bridgeTask.h
 *
 *  Created on: 13.04.2012
 */

#ifndef BRIDGETASK_H_
#define BRIDGETASK_H_

#include <netdevice.h>

struct bridgePath {
	char * name;
	struct netDeviceInterface * bridgeSource;
	struct netDeviceInterface * bridgeDestination;
};

void initBridgeTask( struct bridgePath * );

#endif /* BRIDGETASK_H_ */
