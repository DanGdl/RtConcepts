/*
 * city_services.h
 *
 *  Created on: Jul 10, 2021
 *      Author: max
 */

#ifndef SRC_CITY_SERVICES_H_
#define SRC_CITY_SERVICES_H_


#define DELAY 3000

#define TOTAL_SERVICES 3

// available services
#define FIRE_TEAM 0
#define POLICE 1
#define AMBULANCE 2

// names for available services
#define NAME_FIRE_TEAM "fireTeam"
#define NAME_POLICE "police"
#define NAME_AMBULANCE "ambulance"

// amount of teams in every service
#define FIRE_TEAM_SIZE 2
#define POLICE_SIZE 3
#define AMBULANCE_SIZE 4

const char* names[TOTAL_SERVICES] = {NAME_FIRE_TEAM, NAME_POLICE, NAME_AMBULANCE};
const int   teams[TOTAL_SERVICES] = {FIRE_TEAM_SIZE, POLICE_SIZE, AMBULANCE_SIZE};

typedef struct request {
	int service;
	int groups;
	uint32_t task_time;
} request_t;

typedef struct service_attr {
	osMessageQueueId_t queue;
	osMutexId_t mutex;
	osSemaphoreId_t semaphore;
	const char* name;
	osThreadId_t task;
} service_attr_t;

typedef struct task_params {
	uint32_t time;
	osSemaphoreId_t* semaphore;
	const char* name;
} task_params_t;

#endif /* SRC_CITY_SERVICES_H_ */
