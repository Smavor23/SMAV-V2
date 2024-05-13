#ifndef LABELS_H_
#define LABELS_H_

#define NUM_LANGUAGES 4

char **cmd[NUM_LANGUAGES];
char **ans_P[NUM_LANGUAGES];
char **ans_N[NUM_LANGUAGES];
char **alarm[NUM_LANGUAGES];
static char *lang[] = {"english", "italian", "german", "french"};

enum CMD_IDX{
    STATE_GNRL_CMD = 0,
    TEMP_CMD,
	  T1_CMD,
		T2_CMD,
		H1_CMD,
		H2_CMD,
    STATE_CMD,
    WIND_CMD,
    GAS_CMD,
    BATTERY_CMD,
    ALARM_CMD,
    SET_ALARM_CMD,
    ALARM_ON_CMD,
    ALARM_OFF_CMD,
    RELAY1_CMD,
    RELAY1_ON_CMD, /* FOR GPIO */
    RELAY1_OFF_CMD,
    RELAY2_CMD, 
    RELAY2_ON_CMD, 
    RELAY2_OFF_CMD,
    GET_LANG_CMD, 
    SET_LANG_CMD,
    USRNUM_CMD,
    ADD_CMD,
    DELETE_CMD,
    HELP_CMD,

    /* Any New command should be placed above this point */
    ALERT1_CMD,
    ALERT2_CMD,
    ERROR_CMD,
		INVNUM_CMD,
    LAST_CMD_IDX /* Maps to no command and so used to set if need to to default work in a switch statement */
};

char *cmd_en[] = {"General state","Temperature","T1","T2","H1","H2","State Machine","Wind","Gas","Battery","Alarm Check","Set alarm","Alarm on","alarm off","Relay check","Relay on","Relay off","Burner 2 check","Burner 2 on","Burner 2 off","Language check","Language Set","Active numbers", "add", "delete","Help", "ERASE EEPROM"};
char *ans_P_en[] = {"General State","Temprature","T1","T2","H1","H2","Machine working","Wind","Gas level","External battery:","Alarm is on","Alarm set at","Alarm is on","Alarm is off","Relay on","Relay is on","Relay is off","Burner 2 on","Burner 2 is on","Burner 2 is off","Language is","Language Set ","returns the active user numbers","User Number Added Successfully ","User Number Deleted Successfully ","Help","ERASE EEPROM"};
char *ans_N_en[] = {"0","0","0","0","0","0","Machine not working","Wind","Gas","0K","Alarm is off","0K","0K","Relay Off","Relay off","0K","0K","Relay 2 off","0K","0K","0K","0K","0K","User Number Already Added","User Number do not exist","0","0"};
char *alarm_en[] = {"Machine started","Machine stopped","Machine did not start","Machine error","Low inversion","Windy conditions","Windy conditions solved","Gas level","Battery level low","Error sensor x"};



char *cmd_fr[] = {"Etat general","Temperature","T1","T2","H1","H2","Machine detat","Vent","Gaz","Batterie","Alarme verifier","Regler alarme","Alarme activee","alarme desactivee","Relay verifier","Relay allume","Relay eteint","Bruleur 2 verifier","Bruleur 2 allume","Bruleur 2 eteint","Language check","Langue Definir ","Numeros actifs","ajouter","supprimer","Aider"};
char *ans_P_fr[] = {"Etat general","Temperature ","T1","T2","H1","H2","La machine fonctionne","Vitesse du vent ","Niveau de gaz","Batt externe:","L'alarme est activee","Alarme reglee a","L'alarme est activee","L'alarme est desactivee","Bruleur 1 marche","Le Relay 1 est allume","Le Bruleur 1 est eteint","Bruleur 2 marche","Le Bruleur 2 est allume","Le Bruleur 2 est eteint","La langue est ","Langue definir","renvoie les numeros utilisateurs actifs","Numero d’utilisateur ajoute avec succes","Numero d utilisateur supprime avec succes","Renvoie cette liste"};
char *ans_N_fr[] = {"0","0","0","0","0","0","La machine ne fonctionne pas","Vitesse du vent ","Niveau de gaz","0","L'alarme est desactivee","0K","0K","Relay arret","Relay arret","0K","0K","bruleur 2 arret","0","0","0","0","0","Numero d’utilisateur deja ajoute","Numero d’utilisateur n’existe pas","0"};
char *alarm_fr[] = {"La machine a demarre","Machine arretee","La machine n a pas demarre","Erreur machine","Faible inversion","Conditions venteuses","Conditions venteuses resolues","Niveau de gaz bas","Niveau de batterie faible","Capteur d'erreur x"};


char *cmd_ital[] = {"Condizione","Temperatura","T1","T2","H1","H2","macchina a stati","Il vento","Gas","Batteria","Allarme assegno","Imposta sveglia","Allarme attivato","sveglia spenta","Relay assegno","Relay acceso","Relay spento","Bruciatore 2 assegno","Bruciatore 2 acceso","Bruciatore 2 spento","Language check","lingua Imposta ","Numeri attivi","Aggiungere","cancellare","Aiutare", "ERASE EEPROM"};
char *ans_P_ital[] = {"Condizione","Temperatura","T1","T2","H1","H2","La macchina funziona","Velocita del vento ","Livello del gas","batteria esterna: ","L'allarme e attivato","Allarme impostato su","La sveglia e attiva","La sveglia e disattivata","Relay acceso","Il Relay e acceso","Il Relay 1 e spento","Bruciatore acceso","Il Relay 2 e acceso","Il Bruciatore 2 e spento","La lingua e ","Lingua Imposta","restituisce numeri utente attivi","Numero utente aggiunto correttamente","Numero utente eliminato correttamente","Restituisci questo elenco"};
char *ans_N_ital[] = {"0","0","0","0","0","0","La macchina non funziona","Velocita del vento ","Livello del gas","0","L'allarme e disattivato","0K","0K","0K","Relay spento","0K","0K","Bruciatore 2 spento ","0","0","0","0","0","Numero utente già aggiunto","Numero utente non esiste","0"};
char *alarm_ital[] = {"Macchina avviata","Macchina ferma","La macchina non si avvia","Errore macchina","Bassa inversione","Condizioni ventose","Condizioni ventose risolte","Livello del gas basso","Livello della batteria basso","Errore sensore x"};


char *cmd_deu[] = {"Zustand","Temperatur","T1","T2","H1","H2","Zustandsmaschine","Der Wind","Gas","Schlagzeug","Alarm controle","Wecker einstellen","Wecker aktiviert","Wecker aus","Relay controle","Relay an","Relay aus","Brenner 2 controle","Brenner 2 an","Brenner 2 aus","Language check","Sprache instellen","Aktive Nummern","toevoegen","verwijderen","Helfen"};
char *ans_P_deu[] = {"Zustand","Temperatur","T1","T2","H1","H2","Maschine funktioniert","Windgeschwindigkeit","Gasgehalt","Ext.Batteriespannung: ","Alarm ist ein","Alarm eingestellt auf","Alarm ist an","Alarm ist aus","Relay ein","Relay ist eingeschaltet","Relay ist aus","Brenner 2 ein","Brenner 2 ist eingeschaltet","Brenner 2 ist aus","Sprache ist","Sprache instellen","gibt die aktiven Benutzer Nummern zuruck","Gebruikersnummer met succes toegevoegd","Gebruikersnummer met succes verwijderd","Gibt diese Liste zuruck"};
char *ans_N_deu[] = {"0","0","0","0","0","0","Maschine funktioniert nicht","Windgeschwindigkeit","Gasgehalt","0K","Alarm ist aus ","0K","0K","Relay aus","Relay aus","0K","0K","Relay 2 aus","0K","0K","0K","0K","0K","Gebruikersnummer al toegevoegd","Gebruikersnummer bestaat niet","0"};
char *alarm_deu[] = {"Maschine gestartet","Maschine gestoppt","Maschine startete nicht","Maschinenfehler","Niedrige Inversion","Windige Bedingungen","Windige Bedingungen gelöst","Gaslevel niedrig","Batteriestand niedrig","Fehler Sensor x"};

char *invalid_cmd_error[] = {"error:use these commands: General state,Temperature,State Machine,Wind,Gas,Battery,Alarm Check,Set alarm,Alarm on,alarm off,Relay 1/2 check,Relay 1/2 on/off,Language check,Language Set,Active numbers,Help",
                             "error:questi comandi:Condizione,Temperatura,macchina a stati,Il vento,Gas,Batteria,Allarme assegno,Imposta sveglia,Allarme attivato,sveglia spenta,Relay 1/2 assegno,Relay 1/2 acceso,Relay 1/2 spento,Language check,lingua Imposta ,Numeri attivi,Aiutare",
                             "fout:commando's gebruikt:Zustand,Temperatur,Zustandsmaschine,Der Wind,Gas,Schlagzeug,Alarm controle,Wecker einstellen,Wecker aktiviert,Wecker aus,Relay 1/2 controle,Relay 1/2 an,Relay 1/2 aus,Language check,Sprache instellen,Aktive Nummern,toevoegen,Helfen",
                             "erreur:utilise ces commandes: Etat general,Temperature,Machine detat,Vent,Gaz,Batterie,Alarme verifier,Regler alarme,Alarme activee,alarme desactivee,Relay 1/2 verifier,Relay 1/2 allume,Relay 1/2 eteint,Language check,Langue Definir,Numeros actifs,Aider"};

char *temp_hum[4][2] = {{"Temperature","Humidity"}, {"Temperatura","Umidita"}, {"Temperatur","Luftfeuchtigkeit"}, {"Temperature","Humidite"}};
int languageID;

#ifdef __cplusplus
}
#endif
#endif 
