# uvsh shell

Lightweight shell. Searches in all directories listed in .uvshrc for commands, and will also accept absolute and regular paths to executeables.

#### Compile & Run
    gcc uvshr.c -o uvs
    ./uvs

#### pipe to a file
    do-out command [args] :: outfile.txt
e.g,
    do-out ls -l :: ls_out.txt

#### pipe to another executeable
    do-pipe command [args] :: command [args]

#### TODO
    -Nested pipes
    -Background processes
