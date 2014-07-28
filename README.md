flocate - fast locate replacement (WIP)
=========

Idea behind flocate is to use trigram index to speed up file search.
mlocate linearly scans the database file, which can be slow for large databases.

Current features
---------
- Trigram filter for fast search
- Supports multiple patterns

TODO
---------
- unit tests
- command line options (like help)
- regex support - create a trigram filter from regexp to speed up search
- use compressed data structures for trigram lists
- reduce memory use of database builder
- ultimately GNU locate compatible perhaps?

Performance
------------
Speed up compared to mlocate is noticable. On my laptop:
    $ time ./locate linux debian > /dev/null 
    real  0m0.045s
    user  0m0.044s
    sys 0m0.000s

    $ time locate linux debian > /dev/null
    real 0m0.588s
    user  0m0.556s
    sys 0m0.008s
