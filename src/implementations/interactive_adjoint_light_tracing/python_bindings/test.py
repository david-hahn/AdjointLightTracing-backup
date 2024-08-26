import pyialt
import numpy as np
import time
from matplotlib import pyplot as plt

def optimizeLightsAll( t ):
    lights= t.scene.getLights()
    for l in lights:
        l.optimize().setAll()

def optimizeFirstLightPosition( t ):
    lights= t.scene.getLights()
    for l in lights:
        l.optimize().reset()

    lights[0].optimize().setPositions()

def optimizeLightsOnlyIntensity( t ):
    lights= t.scene.getLights()
    for l in lights:
        opt= l.optimize()
        opt.reset()
        opt.intensity= True


def runGradientDecent(scenePath, iterations):
    t= pyialt.Tamashii()
    t.openScene(scenePath)

    t.showWindow(True)
    t.frame()

    pyialt.log('Start...')

    params= t.scene.lightsToParameterVector()
    for i in range(iterations):
        t.forward(params)
        t.frame()
        phi, grads= t.backward()
        params -= 0.1*grads

    pyialt.log('Done!')

def runFiniteDiffH(scenePath, intensity= False, noBounces= False, log= False):
    pyialt.var.default_camera= 'Camera'
    pyialt.var.numRaysXperLight= 6144
    pyialt.var.numRaysYperLight= 6144
    pyialt.var.bg= (255, 255, 255)
    pyialt.var.constRandSeed= True
    t= pyialt.Tamashii()

    t.openScene(scenePath)

    t.showWindow(True)
    t.frame()

    if noBounces:
        t.ialtBounces= 0

    if intensity:
        optimizeLightsOnlyIntensity( t )

    params= t.scene.lightsToParameterVector()
    originalParams= np.copy(params)
    objPosH= np.full_like(params, 0)
    objNegH= np.full_like(params, 0)

    fdHRange= None
    if log:
        fdHRange= np.array([ pow(10, xi) for xi in np.linspace(-5.0, 0.0, 29) ])
    else:
        fdHRange= np.linspace(1e-5, 1.0, 29)
        
    pyialt.log('Start...')
    
    print(f'params= {params}')

    t.forward(originalParams)
    phi, dp= t.backward()

    print(f'our_grad= {dp}')
    
    k= 1
    for fdH in fdHRange:
        for i in range(len(params)):
            np.copyto(params, originalParams)
            params[i] += fdH
            t.forward(params)
            objPosH[i], dp= t.backward()

            np.copyto(params, originalParams)
            params[i] -= fdH
            t.forward(params)
            objNegH[i], dp= t.backward()
            
        print(f'fd_h({k})= {fdH}')
        print(f'fd_grad({k},:)= {(objPosH- objNegH)/(2.0*fdH)}')
        k += 1

    pyialt.log('Done!')

def runBasicOpt(scenePath, optimizer= pyialt.Optimizers.ADAM):
    pyialt.var.default_camera= 'Camera'
    pyialt.var.numRaysXperLight= 2048
    pyialt.var.numRaysYperLight= 2048
    pyialt.var.bg= (255, 255, 255)
    pyialt.var.constRandSeed= False
    pyialt.var.useSHdiffOnlyCoeffObjective= True
    t= pyialt.Tamashii()

    t.openScene(scenePath)

    #t.showWindow(True)
    t.frame()
    
    stepSize= 1.0
    maxIterations= 100
    
    if optimizer == pyialt.Optimizers.GD:
        stepSize= 0.1
        maxIterations= 50
    elif optimizer == pyialt.Optimizers.CMA_ES:
        maxIterations= 30

    optimizeFirstLightPosition(t)
    
    pyialt.log('Start...')
    
    params= t.scene.lightsToParameterVector()
    t.forward(params)
    t.frame()
    initImage= t.takeScreenshot()
    
    startTime= time.time()    
    history, lastPhi= t.optimize(optimizer, stepSize, maxIterations)
    duration= time.time()- startTime
    
    print(f'Test basic position optimization. p = {params[0:4]}')
    print(f'Duration: {duration}')
    print(f'History size: {len(history)}')
    print(f'History: {history}')
    print(f'Last phi: {lastPhi}')
    
    t.frame()
    resultImage= t.takeScreenshot()
    
    fig, axs = plt.subplots(1, 2, figsize=(10, 10))
    axs[0].imshow(initImage)
    axs[0].axis('off')
    axs[0].set_title('Initial Image')

    axs[1].imshow(resultImage)
    axs[1].axis('off')
    axs[1].set_title('Result Image')
    plt.show()
    
    pyialt.log('Done!')    
    

def main():
    testOfficeScenePath= 'C:/Users/Matthias/Documents/Uni/Bsc/Tamashii/Test-Office-v0.4.gltf'
    simpleOfficeScenePath= 'C:/Users/Matthias/Documents/Uni/Bsc/Tamashii/simple_office_v3_withCamera2/simple_office_v3_withCamera2.gltf'

    #runGradientDecent(testOfficeScenePath, 100)
    #runFiniteDiffH(simpleOfficeScenePath, log= True)
    runBasicOpt(simpleOfficeScenePath)

if __name__ == '__main__':
    main()
