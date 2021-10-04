#%%
from mpl_toolkits.axisartist.parasite_axes import HostAxes, ParasiteAxes
import matplotlib.pyplot as plt
import numpy as np
import time, datetime
import os
import re

# get file name
file_names = os.listdir(os.getcwd())
#print(file_names)

spatial_layers_txt = ""
bitrate_txt = ""
delta_txt = ""
for file_name in file_names:
    #print(file_name)
    if re.search("spatial_layers.txt", file_name):
        spatial_layers_txt = file_name
    elif re.search("bitrate.txt", file_name):
        bitrate_txt = file_name
    elif re.search("delta.txt", file_name):
        delta_txt = file_name

print("spatial_layers_txt is empty!" if len(spatial_layers_txt) == 0 else spatial_layers_txt)
print("bitrate_txt is empty!" if len(bitrate_txt) == 0 else bitrate_txt)
print("delta_txt is empty!" if len(delta_txt) == 0 else delta_txt)

##%%
#parser host.log##########################################
# such as : 0s, type:h264, width:3840, height:2160, fps:1, keyFrameInterval:14, data:103265byte, cpu:3.91%, memory:139mb, bitrates:3000000
file = open(spatial_layers_txt, 'r')

#to 2 dimesion arrary
line = file.readline()
layer_time_array = line.strip().split(',')

layers = []
times_layers = []
#cnt = 0

for item in layer_time_array:
    #print(item)
    if item != '':
        layers.append(int(item.split('-')[0]))
        times_layers.append(int(item.split('-')[1]))
        #cnt=cnt+1
        #if cnt > 10:
            #break
print(layers)
print(times_layers)
file.close()


file = open(bitrate_txt, 'r')
line = file.readline()
bitrate_array = line.strip().split(',')

bitrates = []
times_bitrate = []
#cnt = 0

for item in bitrate_array:
    if item != '':
        bitrates.append(int(item.split('-')[0]))
        times_bitrate.append(int(item.split('-')[1]))
        #cnt=cnt+1
        #if cnt > 80:
            #break

print(bitrates)
print(times_bitrate)
file.close()



file = open(delta_txt, 'r')
line = file.readline()
delta_array = line.strip().split(',')

deltas = []
delta_times = []

for item in delta_array:
    if item != '':
        deltas.append(int(item.split('-')[0]))
        delta_times.append(int(item.split('-')[1]))

#print(deltas)
#print(delta_times)
file.close()


#title is : h264 width:3840 height:2160 keyFrameInterval:14 bitrates:3000000
title = ""

#print(fps)
#data = [int(re.findall(r"\d+\.?\d*",x[6])[0]) for x in linesList]
#print(data)
#cpu = [float(re.findall(r"\d+\.?\d*",x[7])[0]) for x in linesList]
#print(cpu)
#mem = [int(re.findall(r"\d+\.?\d*",x[8])[0]) for x in linesList]
#print(mem)
#print(title)


#create HostAxes(data) and ParasiteAxes(cpu, memory)##########
#figure define, what is 1?
fig = plt.figure(1) 
#use [left, bottom, weight, height]to define axes，0 <= l,b,w,h <= 1
ax_data = HostAxes(fig, [0, 0, 2, 0.9])  

#parasite addtional axes, share x
ax_cpu = ParasiteAxes(ax_data, sharex=ax_data)
ax_mem = ParasiteAxes(ax_data, sharex=ax_data)
#ax_bitrates = ParasiteAxes(ax_data, sharex=ax_data)
#ax_wear = ParasiteAxes(ax_data, sharex=ax_data)

#append axes
ax_data.parasites.append(ax_cpu)
ax_data.parasites.append(ax_mem)
#ax_data.parasites.append(ax_bitrates)
#ax_data.parasites.append(ax_wear)

#invisible right axis of ax_data
ax_data.axis['right'].set_visible(False)
ax_data.axis['top'].set_visible(False)
ax_cpu.axis['right'].set_visible(True)
ax_cpu.axis['right'].major_ticklabels.set_visible(True)
ax_cpu.axis['right'].label.set_visible(True)

#set label for axis
ax_data.set_ylabel('bitrate(kbps)')
ax_data.set_xlabel('time(ms)')
ax_cpu.set_ylabel('layer(0,1,2)')
ax_mem.set_ylabel('delta(ms)')
#ax_bitrates.set_ylabel('bitrates')
#ax_wear.set_ylabel('Wear')

#new axsi line 
load_axisline = ax_mem.get_grid_helper().new_fixed_axis
#cp_axisline = ax_bitrates.get_grid_helper().new_fixed_axis
#wear_axisline = ax_wear.get_grid_helper().new_fixed_axis

#axsi line padding on the right
ax_mem.axis['right2'] = load_axisline(loc='right', axes=ax_mem, offset=(40,0))
#ax_bitrates.axis['right3'] = cp_axisline(loc='right', axes=ax_bitrates, offset=(80,0))
#ax_wear.axis['right4'] = wear_axisline(loc='right', axes=ax_wear, offset=(120,0))

#add host axes to fig
fig.add_axes(ax_data)

#plot fig####################################

#matplotlib散点图点大小_文末送书 | Python绘图，我只用Matplotlib 
#https://blog.csdn.net/weixin_39528029/article/details/111175177
curve_cpu = ax_cpu.scatter(times_layers, layers, label="layer", color='red', s=50) #s为散点图大小

curve_data = ax_data.plot(times_bitrate, bitrates, 'o-', label="bitrate", color='green')
#curve_data = ax_data.scatter(times_bitrate, bitrates, label="bitrate", color='green')
#width = 0.9
#curve_data = ax_data.bar(range(len(times_bitrate)), list(map(float,bitrates)), width,
#                label="bitrate", alpha = .5, color = 'g')

#curve_cpu = ax_cpu.plot(times_layers, layers, label="layer", color='red')
#curve_cpu = ax_data.bar(times_layers, layers, width, label="layer", alpha = .5, color = 'r')

#curve_mem = ax_mem.plot(delta_times, deltas, label="delta", color='blue')
curve_mem = ax_mem.scatter(delta_times, deltas, label="delta", color='blue', s=1)

#curve_bitrates, = ax_bitrates.plot(times, mem, label="bitrates", color='yellow')
#curve_wear, = ax_wear.plot([0, 1, 2], [25, 18, 9], label="Wear", color='blue')
ax_data.legend()

#axes name and color############################
#ax_data.axis['left'].label.set_color(ax_data.get_color())
ax_cpu.axis['right'].label.set_color('red')
ax_mem.axis['right2'].label.set_color('blue')
#ax_bitrates.axis['right3'].label.set_color('yellow')
#ax_wear.axis['right4'].label.set_color('blue')

ax_cpu.axis['right'].major_ticks.set_color('red')
ax_mem.axis['right2'].major_ticks.set_color('blue')
#ax_bitrates.axis['right3'].major_ticks.set_color('yellow')
#ax_wear.axis['right4'].major_ticks.set_color('blue')

ax_cpu.axis['right'].major_ticklabels.set_color('red')
ax_mem.axis['right2'].major_ticklabels.set_color('blue')
#ax_bitrates.axis['right3'].major_ticklabels.set_color('yellow')
#ax_wear.axis['right4'].major_ticklabels.set_color('blue')

ax_cpu.axis['right'].line.set_color('red')
ax_mem.axis['right2'].line.set_color('blue')
#ax_bitrates.axis['right3'].line.set_color('yellow')
#ax_wear.axis['right4'].line.set_color('blue')

#title
fig.suptitle(title, fontsize=16)

#plt.show()

#save png with tight mode#######################
plt.tight_layout()
plt.savefig("statistics.png",bbox_inches='tight')
# %%
