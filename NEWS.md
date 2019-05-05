- All the configuration is done by the `configure` script.
- Building can be done in another directory than the top level directory of the
  source distribution.
- Build shared and static libraries.
- To make Play and Gist libraries compatible with other software and avoid name
  conflicts, exported symbols (functions and variables) and macros have been
  renamed to use more specific prefixes (`pl_` and `PL_` for the public symbols
  of the Play library, `_pl_` for the private symbols of this library).
