# electronics

So far, there are files for EasyEDA electronics design. Files for other electronics design softwares will be added later (when added please let me know if they work in those softwares, I only have access to EasyEDA). I have also inserted the gerber file here.

### if you want to make changes and/or improvements to the project on the electronics side, read this section

You can of course alter any part of this project. If you find something worth addressing, such as something issue with some wiring or maybe just the sensor placement, please let me know.

### if you just want to send the files to the manufacturer and don't worry about it, read this section

The gerber file *Gerber_PCB_TAKEMINAKATA.zip*, aka. the file you send to the manufacturer, is included below, all you need to do is go to JLCPCB website, or any other manufacturer's website and insert it in for them to make.

### parts list

- ESP-32 (38 pin is default on the board, for other board sizes, consult the schematics)
- SCD30
- SCD40
- SGP40 (version with 4 pins)
- SHT85
- DS18
- MQ135

For all components, consult the schematics to make sure they'll fit. For those that don't fit, wire it yourself if you want or consult someone who can do it instead, there is some quidance online that might also help you.
