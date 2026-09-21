# Turn23 isolated split-evidence experiment

Read [PROTOCOL.md](PROTOCOL.md) before [REPORT.md](REPORT.md); the protocol
and C/Python implementations were hash-frozen before the fresh worlds.
The result is material FAIL, independent numerical verification PASS.

Local raw `data/`, `memory/`, `results/` and compiled binaries are ignored
by git but preserved in this checkout. Their four manifests pin every
retained file. A source-only clone can regenerate the deterministic world
family with a private checkout and no preexisting turn23 outputs:

```sh
make -C turns/turn23 all
make -C turns/turn22 all
python3 -B turns/turn23/experiment.py freeze
python3 -B turns/turn23/experiment.py generate
python3 -B turns/turn23/experiment.py extract
python3 -B turns/turn23/experiment.py learn
python3 -B turns/turn23/experiment.py evaluate
python3 -B turns/turn23/verify.py
```

Do not rerun those irreversible stages over this checkout: the scripts use
exclusive receipt writes and preserve the first batch. Gzip container bytes
or local binary hashes may vary after rebuilding; compare decompressed
trace values and measured gains, not historical compressed-file hashes.
This result neither changes the living organism nor measures speech.
