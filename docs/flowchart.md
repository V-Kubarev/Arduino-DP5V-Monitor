# Блок-схема алгоритма работы самодельного дозиметра-радиометра на базе Arduino и ДП-5В

```mermaid
flowchart TD

    %% --- Initialization ---
    START([Power on])
    SETUP[Setup pins, LCD, variables]
    ATTACH[Attach Geiger interrupt]
    LOOP{{Main loop}}

    START --> SETUP --> ATTACH --> LOOP

    %% --- Geiger interrupt ---
    ISR[Geiger pulse interrupt]
    DEAD{Dead time passed?}
    INC[Increment pulse counter]
    BEEP[Set beep flag]

    LOOP -->|interrupt| ISR
    ISR --> DEAD
    DEAD -->|yes| INC --> BEEP --> LOOP
    DEAD -->|no| LOOP

    %% --- Beeper ---
    LOOP --> BEEPCHECK{Beep flag set?}
    BEEPCHECK -->|yes| BUZZER[Activate buzzer]
    BUZZER --> LOOP
    BEEPCHECK -->|no| LOOP

    %% --- Button handling ---
    LOOP --> BUTTON[Read button state]
    PRESS{Press type?}
    RESET[Reset all measurements]
    MODE[Switch display mode]

    BUTTON --> PRESS
    PRESS -->|long press| RESET --> LOOP
    PRESS -->|short press| MODE --> LOOP

    %% --- 1-second processing ---
    LOOP --> TIMER{1 second elapsed?}
    TIMER -->|no| LOOP

    COUNT[Compute pulses per second]
    WINDOW[Update sliding window]
    CPS[Compute CPS]
    DOSE[Compute dose rate]
    ACC[Accumulate dose]
    ERR[Compute statistical error]
    LCD[Update LCD display]

    TIMER -->|yes| COUNT
    COUNT --> WINDOW --> CPS --> DOSE --> ACC --> ERR --> LCD --> LOOP
