# Extinct Birds — Pebble Time 2 watchface

A separate watchface based on the Fish Time software, featuring 22 distinct historical illustrations supplied in `birds (1).zip`.

- One bird per calendar day, changing automatically at local midnight. The bird cannot be changed manually.
- A 7-step modular cycle traverses all 22 birds before repeating; there are no tap gestures or controls.
- Clock and date, common and scientific names.
- Historic artwork color retained, near-white/yellowed paper cleaned toward white.
- No companion app, no network.

## Build and install

`pebble clean && pebble build` then `pebble install --cloudpebble`.

## Screenshot helper

To build a separate preview watchface that advances to a different bird every **10 seconds**:

```bash
python3 scripts/build_screenshots.py
```

To build and install it through CloudPebble:

```bash
python3 scripts/build_screenshots.py --install
```

The helper creates `.screenshot-build/` with its own app UUID and builds there. It does not change the normal daily watchface, which still changes only at midnight. The preview starts with today's bird and advances through all 22 birds, one every 10 seconds, for screenshots. The helper requires the Pebble SDK and Python 3.

## Catalog and attribution

See `bird_catalog.csv` for source filenames, scientific names, and taxonomic qualifications.

Based on Walter Rothschild, *Extinct Birds* (1907), illustrated by John Gerrard Keulemans and others. Historical plates from Wikimedia Commons. Some illustrations depict hypothetical taxa, and one illustrates a possibly-extinct species.

Fork/adaptation of https://github.com/Darachnid/fish-of-new-england-pebble-watchface . Uses a new UUID so the fish watchface is not overwritten.
