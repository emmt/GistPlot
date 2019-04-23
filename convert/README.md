Most of the **initial** conversion work was done by the script `convert.pl`
built from the conversion lists `conv-play.lst` and `conv-gist.lst`.

To get the list of sources and of symbols in Play2 and Gist2:
```sh
cd "$HOME/git/NeoGist"
./tools/list-sources.sh configure Make* nplay ngist etc | gzip -9 > ./tools/nplay-and-ngist-sources.lst.gz
./tools/list-symbols.sh `zcat ./tools/nplay-and-ngist-sources.lst.gz` | gzip -9 > ./tools/nplay-and-ngist-symbols.lst.gz
```

To get the list of sources and of symbols in Play and Gist:
```sh
cd "$HOME/git/yorick"
../NeoGist/tools/list-sources.sh configure *.sh Make* play gist g | gzip -9 > ../NeoGist/tools/play-and-gist-sources.lst.gz
../NeoGist/tools/list-symbols.sh `zcat ../NeoGist/tools/play-and-gist-sources.lst.gz` | gzip -9 > ../NeoGist/tools/play-and-gist-symbols.lst.gz
```

To get the list of sources and of symbols in Yorick:
```sh
cd "$HOME/git/yorick"
../NeoGist/tools/list-sources.sh . | gzip -9 > ../NeoGist/tools/yorick-sources.lst.gz
../NeoGist/tools/list-symbols.sh `zcat ../NeoGist/tools/yorick-sources.lst.gz` | gzip -9 > ../NeoGist/tools/yorick-symbols.lst.gz
```
