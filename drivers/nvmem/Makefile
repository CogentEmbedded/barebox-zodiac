#
# Makefile for nvmem drivers.
#

obj-$(CONFIG_NVMEM)		+= nvmem_core.o
nvmem_core-y			:= core.o

# Devices
obj-$(CONFIG_NVMEM_SNVS_LPGPR)	+= nvmem_snvs_lpgpr.o
nvmem_snvs_lpgpr-y		:= snvs_lpgpr.o

obj-$(CONFIG_RAVE_SP_EEPROM)	+= nvmem-rave-sp-eeprom.o
nvmem-rave-sp-eeprom-y		:= rave-sp-eeprom.o

obj-$(CONFIG_RAVE_SP_CORE)	+= nvmem-rave-sp-parameters.o
nvmem-rave-sp-parameters-y	:= rave-sp-parameters.o

