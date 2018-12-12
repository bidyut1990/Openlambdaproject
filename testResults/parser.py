
# coding: utf-8

# In[1]:


dirt = "G:\\edu\\sem3\\CS 744\\project\\testResults\\noScheduler\\"
file = dirt + 'u3.txt'

with open(file,'r') as fd:
    lines = fd.readlines()


# In[2]:

parts = [lines[x+1:x+3] for x in range(0,len(lines),4)]
parts2 = [[p[0].split(':'),p[1].split(':')] for p in parts]
parts3 = [[int(p[0])*3600+int(p[1])*60+float(p[2][:-1]) for p in p2] for p2 in parts2]
parts4 = [p3[1]-p3[0] for p3 in parts3]
parts4


# In[3]:

import matplotlib.pyplot as plt
from matplotlib import mlab
fig, ax = plt.subplots(figsize=(8, 4))
n_bins = 10
n, bins, patches = ax.hist(parts4, n_bins, normed=1, histtype='step', cumulative=True, label='CDF')
ax.grid(True)
ax.legend(loc='right')
ax.set_title('Cumulative step histograms')
ax.set_xlabel('Duration (s)')
ax.set_ylabel('Likelihood of occurrence')
plt.show()

