# Viikko 6: Robot Framework -testaus

Tama hakemisto kayttaa viikko5:n liikennevalo-ohjelmaa lahtopohjana ja opettajan Robot Framework -sarjaporttipohjaa.

## Ensimmainen testi

1. Rakenna ja flashaa `rtos`-hakemisto nRF-laitteelle.
2. Tarkista Robot-tiedostosta `${COM}`-muuttuja.
3. Kaynnista testi komennolla:

```text
robot serial_str_example.robot
```

## Protokolla

Robot lahettaa yhden komennon, jonka lopussa on `X`:

```text
000120X
```

Laite vastaa samalla lopetusmerkillä:

```text
80X
```

Virheellinen sekuntiarvo `001067X` palauttaa nykyisen parserin virhekoodin `-3X`.
