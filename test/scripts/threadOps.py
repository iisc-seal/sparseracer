import sys
import types
from collections import deque, defaultdict

def withinParen(str):
    return str[str.find('(')+1:str.rfind(')')]

single = {'permit', 'revoke', 'deq', 'end', 'threadexit', 'alloc', 'free', 'read', 'write', 'enterloop', 'exitloop', 'entermonitor', 'exitmonitor', 'wait', 'notify', 'notifyall', 'acquire', 'release', 'reset'}

operations = {'threadinit(', 'threadexit(', 'fork(', 'join(', 'enq(', 'deq(', 'end(', 'permit(', 'revoke(', 'alloc(', 'free(', 'read(', 'write(', 'enterloop(', 
              'exitloop(', 'entermonitor(', 'exitmonitor(', 'acquire(', 'release(', 'wait(', 'notify(', 'notifyall(', 'reset('}

dropped = {'read(', 'write', 'alloc', 'free', 'entermonitor', 'exitmonitor', 'notifyall(', 
           'notify(', 'wait', 'acquire', 'release'}

ins = open(sys.argv[1], "r" )
id = sys.argv[2:]

array = []
discarded = []
retained = []

newToOld = {}

for operation in ins:
  array.append(operation)
ins.close()

for index, operation in enumerate(array):
    args = withinParen(operation).replace(" ","")
    argList = args.split(',')
    
    flag = 0
    for opcode in dropped:
        if opcode in operation:
            flag = 1
            break
   
    if flag == 1:
        continue

    if 'enq' in operation:
        sourceId = argList[0]
        targetId = argList[2]
        if sourceId in id:
            #debugLog.write(str(index)+": "+operation)
            retained.append(str(index)+": "+operation)
            continue

    elif 'fork' in operation or 'join' in operation:    
        sourceId = argList[0]
        targetId = argList[1]
        if sourceId in id or targetId in id:
            #debugLog.write(str(index)+": "+operation)
            retained.append(str(index)+": "+operation)
            continue

    else:
        sourceId = argList[0]
        if sourceId in id:
            #debugLog.write(str(index)+": "+operation)
            retained.append(str(index)+": "+operation)
            continue

f = open(sys.argv[1]+'.only','w')
for op in retained:
   f.write(op)

f.close()
