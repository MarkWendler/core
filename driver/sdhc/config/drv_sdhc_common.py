def instantiateComponent(sdhcComponent):
    
    sdhcSymNumInst = sdhcComponent.createIntegerSymbol("DRV_SDHC_NUM_INSTANCES", None)
    sdhcSymNumInst.setMax(10)
    sdhcSymNumInst.setDefaultValue(1)
    sdhcSymNumInst.setVisible(False)
    
    