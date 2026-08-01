# BBC Micro ROMs

This directory is reserved for local BBC Micro ROM files. ROM binaries are
ignored by Git and are outside every application build target.

The application downloads a user-requested Model B firmware pair from the
[MDFS Acorn MOS ROM repository](https://mdfs.net/System/ROMs/AcornMOS/BBC_120/)
into its private Application Support `BBC Micro ROMS` directory and loads the
bytes at runtime. The manual file-import controls remain available.
