# Pandora PB2 buzzer control-path record

- Source/firmware commit: `e771b99403fe0e3db5fe9eb657da8667f971aa25`
- Target: Pandora STM32L475VE, active buzzer on PB2, active-high
- Image size: 7144 bytes
- BIN/read-back SHA-256: `1b11f7816f42144e8ce163f37a368aa888f8f7d76fcabd04b4a12daa0ed509d9`
- UART capture: 186 bytes; SHA-256 `bd866559bb1ed660730f0b1dab0bf6fc8c60a94f481158cdb5959f2cf189630d`
- ST-Link: `066AFF313933554D43244015`, V2J24S11, STM32L47x/L48x; write verification passed.
- Machine validation: exact firmware identity, short-short-long start/done, final-off; no error marker.
- Human observation: Eugene confirmed hearing the short-short-long pattern and confirmed that it
  stopped after the final long tone.
- Evidence boundary: combined machine and human evidence grants audible B1 for this committed PB2
  fixed pattern. It does not prove sound pressure, frequency accuracy, power, repeated-cycle
  endurance, or broader actuator safety.