/*
 * replay_Builder.c
 *
 *  Created on: Aug 22, 2021
 *      Author: LUIS
 */

#include <Replay_Builder.h>
#include "Labels.h"
#include "string.h"

char replyBuffer[1024];

void flush_buffer()
{
	for(int i=0; i<sizeof(replyBuffer);i++)
	{
		replyBuffer[i] = '\0';
	}
}

void error_buffer()
{
	char* errorBuffer = "Error!";

	flush_buffer();

	for(int i=0; i<strlen((char*)errorBuffer);i++)
	{
		replyBuffer[i] = errorBuffer[i];
	}
}

void concat(char* str)
{
	int index = strlen(replyBuffer);
	int length = strlen(str);
	int size = length + index;

	if (size > sizeof(replyBuffer))
	{
		error_buffer();
	}
	else
	{
		for(int i = 0; i < length; i++)
			replyBuffer[index + i] = str[i];
		replyBuffer[size] = '\0';
	}
}

char* getAlarmMsg(int alarmID)
{
	return "Alarm\n";
}

char* temperature_command(int langID)
{	//	temperature @top and temperature @ ground and dew point in response
	flush_buffer();
	concat(label[0][langID]);
	concat("12.5");
	concat(label[1][langID]);
	concat("22.5");
	concat(label[2][langID]);

	return replyBuffer;
}

char* fuel_command(int langID)
{	//	fuel level %
	flush_buffer();
	concat(label[3][langID]);
	concat("95");
	concat(label[4][langID]);
	return replyBuffer;
}

char* general_command(int langID)
{
	flush_buffer();

	// Temperature

	concat(label[0][langID]);
	concat("12.5");
	concat(label[1][langID]);
	concat("22.5");
	concat(label[2][langID]);
	concat("\r\n");

	// Fuel
	concat(label[3][langID]);
	concat("95");
	concat(label[4][langID]);
	concat("\r\n");

	// Dew Point
	concat(label[5][langID]);
	concat("23");
	concat(label[2][langID]);
	concat("\r\n");

	// Wind speed
	concat(label[6][langID]);
	concat("12");

	// Tank level
	concat(label[7][langID]);
	concat("99");

	// Machine state
	concat(label[8][langID]);
	concat("off");

	return replyBuffer;
}

char* language_command(int langID)
{
	char * replay = label[11][langID];
	return replay;
}

char* start_command(int langID)
{
	char* replay;

	replay = label[13][langID];
//	replay = concat(replay,"10");
//	replay = concat(replay,label[2][langID]);
	return replay;
}

char* alarm_command(int langID)
{
	char* replay;

	replay = label[14][langID];
//	replay = concat(replay,"4422818181");
	return replay;
}

char* getReplayMsg(int replayID)
{
	char* replyString;
	int langID;

	langID = get_language();
	switch (replayID)
	{
	case 0:
		replyString = temperature_command(langID);
		break;
	case 1:
		replyString = fuel_command(langID);
		break;
	case 2:
		replyString = general_command(langID);
		break;
	case 3:
		replyString = language_command(langID);
		break;
	case 4:
		replyString = start_command(langID);
		break;
	case 5:
		replyString = alarm_command(langID);
		break;

	default:
		error_buffer();
		replyString = replyBuffer;
		break;
	}
	return replyString;
}
