
principi di funzionamento gnerale.

RPY cicla.                   Arduino cicla
readingDataFromArduino     readingCommandsFromRPY
                           # reply --> readingDone (series of commands needs to have a EOT End Of Tranmission Char)
  .                                .
  .                                .
  .                                .
  .                                .
sendingCommandsToArduino   sendingDataToArduino


RPY lo faccio che cicla sempre.
Se per un po' di cicli salta la comunicazione no problem e uso i dati precedenti/uso flag di communicationUpdated = True
Faccio un conteggio oltre il quale se arduino non risponde allora triggero messaggio di comunicazione e mando un reset.

Arduino pure cicla sempre. Se non ha comandi fa le sue cose di monitoraggio e mi ritorna semplicemente i dati.
Se deve fare cose, allroa rimarrà impegnato nella sua routine che fa cose e si, in questo caso, prolungherà un po' il 
tempo di esecuzione (e quindi di risposta nella comunicazione).

Voglio leggere più comandi in serie. Quindi mi servono dei while(Serial.available() > 0). 
Per fare questo ho bisogno di terminatori EOT End Of Tranmission e dei sengali di sincronizzazione

durante il corso del programma faccio una lista commandsToSend, che passo passo popolo con le cose che devo fare.
commandsToSend.append['xx']. Ovviamente controllo sulla lunghezza massima + eventuale memoria/secondo array di comando.


Commands are from RPY --> Arduino
Readings are from Arduino --> RPY

Commands in format:
let's use 2 letters to identify commands
26 letters on the keyboards * 2 with Maiusc = 52 letters
52*52 = 2704 possibile combinations of commands

If need to set numbers: what follows the _is the number to be used, according to the command.

#xx_123

## COMMAND ID ## DESCRIPTION ##
      aa            RPY authorizing INO to proceed forward in the code
      nc            No commands. RPY doesn't have to send nothing to INO
      ac


#001 the following command is #34 (value t be setted in the corresponding parameter)

Series of commands:
#GNRL001#SET001#34

Which is the maximum length of series of command I can send?
