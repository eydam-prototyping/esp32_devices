idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES spi_flash network_manager http_server
                       INCLUDE_DIRS "")

# Create SPIFFS image from spiffs_image folder
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
