# Manifest: DIY Dosimeter-Radiometer on Arduino & DP-5V

## Project Identity

| Field | Value |
|-------|-------|
| **Project Name** | DIY Dosimeter-Radiometer |
| **Based on** | Arduino + DP-5V military survey meter |
| **Current Version** | 0.3.0 |
| **License** | GNU General Public License v3.0 |
| **Project Type** | Open Source Hardware/Software |
| **Target Audience** | Makers, hobbyists, physics educators, electronics enthusiasts |

---

## Core Purpose

To transform a Soviet-era military radiation survey meter (DP-5V) into a modern, digital dosimeter-radiometer with accurate measurement capabilities, statistical analysis, and an intuitive LCD interface — while preserving educational value and experimental flexibility.

---

## Design Philosophy

### 1. **Educational First**
The project prioritizes understanding over black-box functionality. Every algorithm is documented, explained, and made transparent. Users should learn *why* things work, not just *that* they work.

### 2. **Scientific Accuracy**
Despite being a DIY project, measurements must be statistically sound:
- Proper dead time compensation
- Poisson statistics for error estimation
- Calibration against reference instruments
- Transparent limitations and assumptions

### 3. **Practical Usability**
- One-button interface for simplicity
- Clear, readable LCD display
- Audible/visual pulse feedback
- Two display modes (instant rate + accumulated dose)
- Automatic adaptation to radiation levels

### 4. **Open Development**
- Full source code availability
- Detailed documentation
- Modular architecture for experimentation
- Version control and changelog

---

## Core Algorithms

| Algorithm | Purpose | Key Innovation |
|-----------|---------|----------------|
| Hardware Interrupts | Pulse counting | Zero missed counts, even at high rates |
| Software Dead Time | Signal filtering | Eliminates false counts from pulse "ringing" |
| Dynamic Sliding Window | CPS averaging | Adapts window size (5s/30s) based on radiation level |
| Dead Time Compensation | Physical correction | Mathematical correction for tube recovery time |
| Poisson Statistics | Error estimation | Scientific uncertainty measurement |
| Dose Integration | Cumulative tracking | Running total of accumulated dose |

---

## Technical Stack

- **Microcontroller:** ATmega328P (Arduino Uno/Nano)
- **Programming Language:** C++ (Arduino framework)
- **Display:** LCD 16×2 with I2C
- **Input:** Single button with long/short press detection
- **Output:** Piezo buzzer, LED, LCD
- **Signal Source:** DP-5V audio output (with matching circuit)

---

## Version Philosophy

### v0.x — Experimental Phase
- Core algorithms proof-of-concept
- Basic functionality implemented
- Subject to significant changes
- Community feedback welcome

### Future v1.0 — Stable Release
- All core features mature
- API stability
- Comprehensive documentation
- Multiple hardware variants supported

---

## Development Principles

1. **KISS (Keep It Simple, Stupid)** — Avoid unnecessary complexity
2. **Document as you code** — Every function has a purpose, every purpose is explained
3. **Test with real hardware** — Theory must work on actual DP-5V units
4. **Respect the hardware** — Proper interfacing, no damage to vintage equipment
5. **Scientific rigor** — Measurements must be defensible

---

## Scope Boundaries

### ✅ **In Scope**
- Geiger pulse counting and processing
- Dose rate calculation (µSv/h)
- Accumulated dose tracking
- Statistical error estimation
- LCD user interface
- Single-button interaction
- Calibration against reference

### ❌ **Out of Scope**
- Certified safety measurements
- Medical applications
- Regulatory compliance
- Professional metrology
- Commercial production

---

## Success Metrics

1. **Accuracy:** ±20% of reference instrument after calibration
2. **Stability:** Smooth readings on stable background
3. **Responsiveness:** <10 seconds to detect significant increase
4. **Usability:** Intuitive enough for first-time user
5. **Documentation:** Complete coverage of all algorithms
6. **Reliability:** 24/7 continuous operation possible

---

## Community Values

- **Open collaboration** — All contributions welcome
- **Respectful discourse** — No toxic behavior
- **Learning together** — Questions encouraged
- **Sharing knowledge** — Document findings
- **Attribution** — Credit where due

---

## Current Status

**Version 0.3.0 — Feature Complete**
- ✅ Hardware interrupt counting
- ✅ Software dead time filtering
- ✅ Dynamic sliding window averaging
- ✅ Dead time compensation
- ✅ Poisson error estimation
- ✅ Dose accumulation
- ✅ Dual display modes
- ✅ Long/short press button control
- ✅ I2C LCD output
- ✅ Calibration support

**In Progress**
- Additional display formats
- Data logging capability
- Low-power modes
- Multiple tube type support

---

## Project Structure

```
/
├── firmware/               # Arduino sketches
│   └── arduino/
│       ├── geiger_0.2.0/
│       └── geiger_0.3.0/
├── docs/                   # Documentation
│   ├── SOFTWARE_RU.md      # Russian algorithm docs
│   ├── SOFTWARE_EN.md      # English algorithm docs
│   ├── architecture.md     # System architecture
│   ├── flowchart.md        # Algorithm flowchart
│   ├── math_model.md       # Mathematical foundations
│   ├── LIMITATIONS_RU.md   # Known limitations (RU)
│   ├── LIMITATIONS_EN.md   # Known limitations (EN)
│   └── TODO.md             # Future improvements
├── assets/                 # Images and resources
│   ├── device_photo_400.jpg
│   └── connection_diagram_600.jpg
├── README.md               # This file
└── manifest.md             # Project manifesto
```

---

## Acknowledgments

- The open-source Arduino community
- Soviet engineers who designed the DP-5V
- Contributors who test and improve the code
- Educators using this for physics demonstrations
- Everyone who documents their experiments

---

*"Make it work, make it right, make it fast — in that order."*  
— Traditional engineering wisdom

---

**Version 0.3.0** | Last updated: February 2026 | [GitHub Repository](#)