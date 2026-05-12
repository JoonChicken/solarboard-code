idf_component_register(SRCS "solarboard/solarboard.cpp" "sd.cpp" "main.cpp"
                    "i2c/I2C.cpp"
                    "i2c/BMP581.cpp"
                    "i2c/BMI323.cpp"
                    "spi/SPI.cpp"
                    "spi/INA229Q1.cpp"
                    PRIV_REQUIRES
                            spi_flash
                            esp_driver_i2c
                            esp_timer
                            fatfs
                            sdmmc
                            esp_driver_spi
                            esp_driver_sdspi
                    INCLUDE_DIRS "."
                )

