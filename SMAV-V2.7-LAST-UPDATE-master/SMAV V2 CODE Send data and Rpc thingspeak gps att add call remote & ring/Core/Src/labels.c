/*
 * labels.c
 *
 *  Created on: Aug 22, 2021
 *      Author: LUIS
 */

#include "labels.h"

int get_language()
{
	if (languageID > 1 || languageID < 0)
	{
		languageID = 0;
	}
	return languageID;
}

void set_language(int id)
{
	languageID = id;
}

void initialize_labels()
{
	// Commands definition
	command[0][0] = "temperature";
	command[0][1] = "temperatura";

	command[1][0] = "fuel";
	command[1][1] = "combustible";

	command[2][0] = "general";
	command[2][1] = "general";

	command[3][0] = "language";
	command[3][1] = "idioma";

	command[4][0] = "start";
	command[4][1] = "arranque";

	command[5][0] = "alarm";
	command[5][1] = "alarma";

	// Replys text definitions
	label[0][0] = "Temperature at top: ";
	label[0][1] = "Temperatura superior: ";

	label[1][0] = "C. temperature at ground level: ";
	label[1][1] = "C. Temperatura a nivel del suelo: ";

	label[2][0] = "C.";
	label[2][1] = "C.";

	label[3][0] = "Fuel level: ";
	label[3][1] = "Nivel del combustible: ";

	label[4][0] = "%.";
	label[4][1] = "%.";

	label[5][0] = "Dew point: ";
	label[5][1] = "Dew point: ";

	label[6][0] = "Wind speed: ";
	label[6][1] = "Velocidad delviento: ";

	label[7][0] = "Km/h. \r\nGas tank level: ";
	label[7][1] = "Km/h. \r\nNivel del tanque de combustible: ";

	label[8][0] = "% \r\nMachine is ";
	label[8][1] = "% \r\nLa maquina esta ";

	label[9][0] = "off.";
	label[9][1] = "apagada.";

	label[10][0] = "on.";
	label[10][1] = "prendida.";

	label[11][0] = "The language was set to English";
	label[11][1] = "El idioma fue cambiado a Español";

	label[12][0] = "The language is set to English";
	label[12][1] = "El idioma seleccionado es Español";

	label[13][0] = "The start temperature was set to: ";
	label[13][1] = "La temperatura de arranque seleccionada es: ";

	label[14][0] = "The alarm number was set to: ";
	label[14][1] = "El numero de la alarma seleccionado es: ";

	// Alarms text definition
	alarm[0][0] = "Tank level below 30%.";
	alarm[0][1] = "Nivel del tanque por de bajo del 30%.";

	alarm[1][0] = "Tank level (still) below 20%.";
	alarm[1][1] = "Nivel del tanque por debajo del 20%.";

	alarm[2][0] = "Please switch your machine off, tank level below 10%.";
	alarm[2][1] = "Por favor apague el equipo, nivel del tanke por debajo del 10%.";

	alarm[3][0] = "Machine not running at start temperature";
	alarm[3][1] = "Equipo no enciende a la temperatura de arranque";

	alarm[4][0] = "start temperature.";
	alarm[4][1] = "temperatura de arranque.";

	alarm[5][0] = "Your inversion is very low please start earlier than normal.";
	alarm[5][1] = "Su inversion es muy baja, por favor arranque antes de lo habitual.";

	alarm[6][0] = "The dew point around the machine is very low, please start your machine earlier";
	alarm[6][1] = "El \"Dew point\" de su maquina es muy bajo, por favor arranque antes de lo habitual.";

	alarm[7][0] = "Attention the wind speed is ";
	alarm[7][1] = "Atencion, la velocidad del viento es ";

	alarm[7][0] = " km/Hr. Better delay the start of the machine.";
	alarm[7][1] = "km/Hr. Se recomienda arrancar mas tarde el equipo.";

	alarm[7][0] = "The wind speed is 15 km/hr now, start your machine with care.";
	alarm[7][1] = "La velocidad del viento es de 15km/hr, arranque su equipo con precaucion.";

}
