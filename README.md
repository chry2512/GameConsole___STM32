STM32 Retro Game Console (Snake)

## Panoramica del Progetto
Questo progetto implementa una console di gioco portatile embedded basata su microcontrollore **STM32F407VET6** (Cortex-M4/68 MHz) con sistema operativo real-time **FreeRTOS**.
La console esegue il celebre gioco **Snake**, arricchito da grafica personalizzata a colori , controllo analogico tramite joystick a 2 assi, interazione touchscreen resistiva, feedback visivo tramite LED di stato, salvataggio persistente su memoria Flash SPI esterna (W25Qxx) e monitoraggio ambientale in tempo reale tramite sensore  BME280.


## Architettura Hardware 
- **Microcontrollore Principale**: STM32F407VET6.

- **Display Grafico**: Display TFT a colori 3.2" , pilotato tramite bus parallelo ad alte prestazioni **FSMC** (Flexible Static Memory Controller).

- **Touch Screen Controller**:  su bus **SPI2**.

- **Memoria Non-Volatile Esterna**: Flash SPI su bus **SPI1** per memorizzazione di asset grafici e classifica (Leaderboard persistente).

- **Sensore Ambientale**: **BME280** (temperatura, pressione atmosferica, umidità relativa) su bus **I2C1**.

- **Input Giocatore**:
  - Joystick analogico a 2 assi (X, Y) campionato tramite **ADC1 con DMA**.
  - Pulsante integrato nel joystick (push switch) su GPIO con pull-up.
  - Pulsante di accensione/standby (Power button) con interrupt esterno **EXTI**.
  - Schermo tattile .

- **Feedback & Debug**:
  - LED di stato (ON LED, ERROR LED).
  - Porta seriale di debug e telemetria su **USART1**.


## Protocolli di Comunicazione & Interfacce Seriali

La **comunicazione seriale** è un metodo di trasferimento di dati in cui i bit vengono inviati uno dopo l'altro su un unico filo di comunicazione.


### 1. SWD (Serial Wire Debug)
- **Definizione Tecnica**: Protocollo standard a 2 fili (SWDIO per dati bidirezionali, SWCLK per il clock) definito da ARM per la programmazione  e il debug dei processori Cortex-M. Rispetto al JTAG tradizionale (4-5 pin), riduce drasticamente l'occupazione dei pin mantenendo pieno supporto .

- **Utilizzo nel Progetto**: Utilizzato tramite interfaccia ST-LINK V2  (pin `PA13` e `PA14`) per il caricamento del firmware binario e il debug in tempo reale su VS Code.

### 2. UART / USART (Universal Synchronous/Asynchronous Receiver-Transmitter)
- **Definizione Tecnica**: Utilizzato per la comunicazione punto-punto tra due dispositivi, UART trasmette dati asincronamente, cioè senza un segnale di clock condiviso tra mittente e destinatario.

- **Utilizzo nel Progetto**: 
  - **USART1** (TX su `PA9`, RX su `PA10`).
  - Utilizzata per l'output di diagnostica e telemetria real-time (tramite reindirizzamento di `printf`), logging dello stato della console, punteggio di gioco, coordinate touch e misurazioni ambientali.

### 3. SPI (Serial Peripheral Interface)
- **Definizione Tecnica**: Protocollo seriale sincrono che supporta la comunicazione full-duplex ad alta velocità tra un master e uno o più dispositivi slave. Richiede quattro fili di comunicazione: MOSI, MISO, SCLK e SS.

- **Utilizzo nel Progetto**:
  - **SPI1 (PB3=SCK, PB4=MISO, PB5=MOSI, PB0=CS)**: Dedicata alla **memoria Flash esterna**. 
  Opera a velocità elevata per caricare texture/sprite grafici e memorizzare l'array dei punteggi record (Leaderboard).

  - **SPI2 (PB13=SCK, PB14=MISO, PB15=MOSI, PB12=CS_T, PC5=INT/PENIRQ)**: 
  Dedicata al **Touch Controller**. 
  Il microcontrollore interroga il controller convertendo la resistenza dello schermo in coordinate X/Y a 12-bit, supportato dal pin di interrupt `PC5` che segnala il tocco fisico.

### 4. I2C (Inter-Integrated Circuit)
- **Definizione Tecnica**: Utilizzato per la comunicazione tra più dispositivi su un singolo bus, I2C è un protocollo seriale multi-master che richiede due fili di comunicazione: SDA e SCL.

- **Utilizzo nel Progetto**:
  - **I2C1 (PB6=SCL, PB7=SDA)**: Utilizzato per comunicare con il sensore **BME280**.
  - Il sensore opera in **Forced Mode** per annullare l'autoriscaldamento del chip. I campionamenti ambientali (temperatura, pressione atmosferica, umidità) e le stampe UART avvengono in modo dedicato durante la schermata di standby (`CONSOLE_STATE_OFF`), per poi disattivarsi durante le fasi attive di gioco e non consumare risorse di calcolo.

### 5. FSMC (Flexible Static Memory Controller) - Bus Display Parallelo

- **Definizione Tecnica**: Controller hardware integrato nell'STM32F4 che mappa periferiche di memoria esterne (SRAM, NOR Flash, LCD) direttamente nello spazio di indirizzamento della CPU (Bank 1). 

- **Utilizzo nel Progetto**: Pilota il display LCD TFT 320x240 con bus dati a **16-bit**. Consente di aggiornare lo schermo ad altissimo framerate (30+ FPS) senza alcun rallentamento della CPU.


## Fondamenti Tecnologici 

### 1. GPIO (General-Purpose Input/Output)
- **Definizione Tecnica**: Le GPIO (General Purpose Input/Output) sono i canali di comunicazione di STM32 con il mondo esterno. Sono pin digitali configurabili via software in modalità Input o Output, oltre a modalità alternate (AF) per instradare segnali periferici.

- **Utilizzo nel Progetto**: Gestione diretta dei Chip Select (`PB0` per Flash, `PB12` per Touch), pilotaggio dei LED di stato (`PA6`, `PA7`), lettura del pulsante joystick (`PE1`) e pin di interrupt.

### 2. Sistemi Real-Time (RTOS)

- **Definizione Tecnica**: Sistemi operativi in cui la correttezza del programma non dipende solo dal risultato logico del calcolo, ma anche dal rispetto rigoroso dei vincoli temporali (**deadline**). Utilizzano uno scheduler preemptive basato su priorità per garantire determinismo e tempi di latenza prevedibili, dividendo l'applicazione in thread indipendenti (Task).

- **Utilizzo nel Progetto**: Implementato tramite **FreeRTOS** per orchestrare in modo parallelo e deterministico il rendering grafico a ~30 FPS, la logica di gioco a 50 Hz, il campionamento analogico dei comandi e la telemetria I2C, prevenendo blocchi della CPU.

### 3. Interrupt (EXTI & NVIC)

- **Definizione Tecnica**: Segnali asincroni hardware o software che sospendono temporaneamente il flusso di esecuzione ordinario del processore per eseguire una routine dedicata (**ISR** - Interrupt Service Routine), gestiti tramite il controllore prioritario vettorizzato (**NVIC**).

- **Utilizzo nel Progetto**: Linea **EXTI3** (`PE3`) per l'accensione istantanea dal pulsante Power, linea **EXTI9_5** (`PC5`) per la rilevazione immediata della pressione del Touch Screen (PENIRQ), e interrupt di fine trasferimento DMA per l'ADC.

### 4. HAL (Hardware Abstraction Layer)
- **Definizione Tecnica**: Strato software intermedio fornito da STMicroelectronics che astrae i registri a basso livello del silicio fornendo API C standardizzate e portabili per configurare, avviare e gestire le periferiche hardware del microcontrollore.

- **Utilizzo nel Progetto**: Utilizzato per inizializzare e controllare in sicurezza i bus di comunicazione (`HAL_SPI_*`, `HAL_I2C_*`, `HAL_UART_*`), la gestione dei pin (`HAL_GPIO_*`), le conversioni analogiche e la gestione energetica.

## Architettura Software Real-Time (FreeRTOS)

Il software è strutturato secondo una pipeline deterministica a task cooperativi e preemptive tramite **FreeRTOS**:

| Task Name | Priorità | Periodo | Responsabilità Principale |

| **RenderTask** | Normal | ~33 ms (~30 FPS) | Rendering grafico double-buffering parziale su FSMC LCD (frame rate costante). |

| **GameLogicTask** | Normal | 20 ms (50 Hz) | Macchina a stati della console (`OFF`, `LOAD`, `START`, `NAME`, `DIFFICULTY`, `GAME`, `LEADERBOARD`), collisioni e logica di Snake, gestione eventi touch e pulsanti. |

| **InputTask** | AboveNormal | 20 ms (50 Hz) | Campionamento continuo ADC1 (DMA) del joystick, calibrazione della zona morta (deadzone), filtraggio e accodamento eventi. |

| **SensorTask** | Low | 2000 ms | Campionamento I2C del sensore BME280 in stato `OFF` con compensazione delle formule di calibrazione Bosch (Integer 64-bit e float) e stampa metrica. |

### Meccanismi di Sincronizzazione RTOS:
- **`gameMutex`**: Mutex binario ricorsivo che protegge l'accesso concorrente alle strutture dati condivise di stato gioco (`SnakeGame_t`) e console (`Console_t`) tra il task di logica e il task di rendering grafico.

- **`joystickQueue`**: Coda di messaggi (`osMessageQueue`) thread-safe per inviare i vettori direzionali elaborati da `InputTask` a `GameLogicTask`.

---

# Macchina a Stati della Console 

1. **OFF**: Schermo nero a basso consumo con orologio e meteo (temperatura e pressione atmosferica misurate da BME280). Toccando lo schermo o premendo il tasto Power si avvia la console.

2. **LOAD**: Schermata di boot con logo .

3. **START**: Schermata iniziale "Press Any Key / Touch Screen to Play".

4. **NAME**: Schermata di inserimento nome giocatore  tramite joystick.

5. **DIFFICULTY**: Selezione difficoltà (Easy, Medium, Hard).

6. **GAME**: Partita attiva di Snake .

7. **LEADERBOARD**: Classifica dei punteggi migliori salvata su Flash SPI esterna.

## Video(Demo)
In questa sezione sono raccolte le dimostrazioni video del funzionamento della console e dei singoli protocolli hardware implementati:

1.  **Main GamePlay**: 
   <!-- [Link Video 1  -->
2. **SWD (Serial Wire Debug)**: Debug Seriale
   <!-- [Link Video 2 --->
3. **UART Telemetry & Logs**: Monitor seriale 
   <!-- [Link Video 3  -->
4. **I2C - BME280 Environmental Sensor**: Rilevazione live di temperatura, pressione 
   <!-- [Link Video 4  -->
5. **SPI - External Flash (W25Qxx)**: Lettura degli asset grafici with/any internal asset
   <!-- [Link Video 5 -->
6. **SPI -  Touch Screen (XPT2046)**:
   <!-- [Link Video 6 -->
7. **Error Handling**:

8. ** Stop Mode**
   
https://github.com/user-attachments/assets/10bd0c1f-33fd-4cc3-895b-ed0fa7daf5c0




