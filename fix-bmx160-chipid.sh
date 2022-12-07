#!/bin/bash

if grep -q "UINT8_C(0xD8)" libs/BMI160_driver/bmi160_v3.9.1/bmi160_defs.h; then
    echo "BMX160 ChipID already fixed"
else
    rm -rf libs/BMI160_driver/bmi160_v3.9.1/examples
    git apply --reject --whitespace=fix fix-bmx160-chipid.patch
    echo "BMX160 ChipID fixed"
fi
