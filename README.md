# Rogue 5.2

Reconstructed source for Rogue 5.2 (Toy, Arnold, Wichman; Berkeley 1982).
Compiled with a modified 4.1BSD toolchain, it byte-exactly reproduces
the installed binary `4.1.snap/usr/lasttape/usr/games/rogue`
(md5 `7160b215bcb7708b830bb189f78313b9`) from CSRG Archive CD-ROM Disc 1.
`rogue.me` is the guide source (`4.1c.2/usr/doc/rogue`,
md5 `2dff7683a67af0749ab3279a99a4a0cb`). `rogue.6` is the manual page from
4.1BSD (`4.1.snap/usr/man/man6/rogue.6`,
md5 `f84eef587bc38bc0823f2cd029df030b`).

The binary was stripped, so identifier names are inferred (mostly from other
rogue sources); comments and whitespace are likewise inferred from available
sources, as is most of the Makefile.
