### 课堂题目要求
1.实现NPC AI按照指定路线巡逻。<br>
2.实现NPC寻路过程中，累了主动去几个随机位置休息。<br>
3.实现NPC遇到玩家以后主动和玩家进行打招呼。<br>

### 实现玩法:
1.实现了NPC AI按照指定路线巡逻。<br>
-> 实现方式为 设定几个actor之后使用movetoactor<br>
-> 加入场景中的navgation<br>
2.实现了NPC寻路过程中，累了主动去几个随机位置休息。<br>
-> 实现方式为 设定几个定时器判断是否需要休息和是否正在休息<br>
-> 如果需要休息则跑步去指定位置休息,休息时间为5s<br>
-> 如果正在休息则将速度置为0<br>
3.实现了NPC遇到玩家以后主动和玩家进行打招呼。<br>
-> 实现方式为 使用overlap函数判断是否与玩家发生重叠,如果重叠,进入打招呼模式<br>
-> 如果当前正在巡航,那打断当前巡航<br>
-> 打招呼2s钟结束后发布下一次目的地的点<br>

### 默认每20s休息5s,打招呼时间为2s,从路上到休息点的时间不计入休息时间


### 部分源码放至source文件夹中


