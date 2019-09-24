import glob
import matplotlib.pyplot as plt
import numpy as np

folderName = "analyse_mesher_1557901/"
startLevel = 6
maxRL = 10
nbThreads = 12
nbTries = 10

#dataA = {} # list indexed by nbThread of dictionnary
# exemple : dataA = [ { "customReductionTBB" : [ list indexed by level, contains meanTime] }  ]
#dataS = {}


def getData(option):
    data = {}

    for nbThread in range(1,nbThreads+1):
    
        data[nbThread] = {}
        files = glob.glob(folderName + "analyse_mesher_" + option + "/thread_" + str(nbThread) + "/*")
        files += glob.glob(folderName + "analyse_mesher_" + option + "/sequential.txt")

        for fileStr in files:

            #if 'eduction' not in fileStr or 'sequential' in fileStr:
            if 'TBBV3' in fileStr or 'sequential' in fileStr:

                file = open(fileStr, 'r')

                methodName = fileStr[:-4].split("/")[-1]

                #Sum of all time for each level
                data[nbThread][methodName] = [0 for i in range(maxRL)]

                for line in file:
                    line = line.split(' ')
                    if line[0] == "Level" and int(line[1]) < maxRL:
                        data[nbThread][methodName][int(line[1])] += int(line[3])

        #Calculate mean
        for method in data[nbThread]:
            if method != "sequential":
                data[nbThread][method] = [x / nbTries for x in data[nbThread][method]]

    #Get sequential
    #file = open(folderName + "analyse_mesher_" + option + "/sequential.txt")
    #data[0] = []
    #for line in file:
    #    line = line.split(' ')
    #    if line[0] == "Level":
    #        data[0].append(line[3])

    return data

dataA = getData('a')
dataS = getData('s')



def plot(data, opt):

    levels = [i for i in range(maxRL)]

    counter = 1

    #fig = plt.figure()
    #fig.suptitle("Graph of average thread execution time for different number of elements", fontsize=16)

    for nbThread in range(6,nbThreads+1,2):

        t = " thread" if nbThread == 1 else " threads"
        plt.figure(counter)
        plt.title("Option -" + opt + " with " + str(nbThread) + t)
        plt.ylabel("Execution time(ms)")
        plt.xlabel("Level number")

        #Fullscreen:
        #mng = plt.get_current_fig_manager()
        #mng.resize(*mng.window.maxsize())

        for methodName, timesList in data[nbThread].items():

            #print(timesList)

            if timesList[-1] == 0:
                continue

            plt.plot(levels, timesList, '-x', label=methodName, markersize=15)
            plt.text(levels[-1] + 0.5, timesList[-1], str(timesList[-1]))


        #plot sequential
        #There is a problem !! When plot sequential, 
        #all yvalues are not shown correctly
        #WHY ????
        #plt.plot(levels, data[0])
        #plt.text(levels[-1], data[0][-1], str(data[0][-1]))

        plt.legend()
        counter += 1

    plt.show()


#plot(dataA, 'a')
#plot(dataS, 's')


def plotMutex(data, opt):

    levels = [i for i in range(startLevel, maxRL)]

    counter = 1

    plt.figure(counter)
    plt.title("Option -" + opt + " mutex version")
    plt.ylabel("Speed up")
    #plt.yscale("log")
    plt.xlabel("Level number")

    seq_draw = False


    for nbThread in range(4,nbThreads+1,4):

        #Fullscreen:
        #mng = plt.get_current_fig_manager()
        #mng.resize(*mng.window.maxsize())



        for methodName, timesList in data[nbThread].items():

            #print(timesList)

            timesList = timesList[startLevel:]


            if seq_draw and "sequential" in methodName:
                continue

            if timesList[-1] == 0:
                continue
            else:
                print(methodName, timesList)

            label = "parallel - " + str(nbThread) + " threads"

            if "sequential" in methodName:
                seq_draw = True
                label = methodName

            timesList = [data[nbThread]["sequential"][x] / timesList[x - startLevel] for x in range(startLevel, startLevel + len(timesList))]

            plt.plot(levels, timesList, '-x', label=label, markersize=15)
            plt.text(levels[-1] + 0.2, timesList[-1] if nbThread != 12 else timesList[-1], str(timesList[-1]))


        #plot sequential
        #There is a problem !! When plot sequential,
        #all yvalues are not shown correctly
        #WHY ????
        #plt.plot(levels, data[0])
        #plt.text(levels[-1], data[0][-1], str(data[0][-1]))

        plt.legend()
        counter += 1

    plt.show()


plotMutex(dataA, 'a')
plotMutex(dataS, 's')
