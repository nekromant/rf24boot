for c in `ls production_configs/`; do \
 cp production_configs/$c .config; \
 make clean; \
 make build; \
 cp images/*.hex prebuilt/; \
 cp .config production_configs/$c; \
done

