# Copyright (c) 2019, University of Washington All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
# Redistributions of source code must retain the above copyright notice, this list
# of conditions and the following disclaimer.
# 
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
# 
# Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
import torchvision
import os

# Network
class LeNet5(nn.Module):
  """
  LeNet-5

  https://cs.nyu.edu/~yann/2010f-G22-2565-001/diglib/lecun-98.pdf
  (Page 7)
  """
  def __init__(self):
    super(LeNet5, self).__init__()

    self.conv = nn.Sequential(
      nn.Conv2d(1, 6, kernel_size=(5,5)),
      nn.ReLU(),
      nn.MaxPool2d(kernel_size=(2,2), stride=2),
      nn.Conv2d(6, 16, kernel_size=(5,5)),
      nn.ReLU(),
      nn.MaxPool2d(kernel_size=(2,2), stride=2),
      nn.Conv2d(16, 120, kernel_size=(5,5)),
    )

    self.fc = nn.Sequential(
      nn.Linear(120, 84),
      nn.ReLU(),
      nn.Linear(84, 10),
      nn.LogSoftmax(dim=-1),
    )

  def forward(self, data):
    x = self.conv(data)
    x = x.view(x.shape[0], -1)
    x = self.fc(x)
    return x

# Train routine
def train(net, loader, epochs, optimizer, loss_func):
  print('Training {} for {} epoch(s)...\n'.format(type(net).__name__, epochs))
  for epoch in range(epochs):
    losses = []

    for batch_idx, (data, labels) in enumerate(loader, 0):
      batch_size = len(data)
      optimizer.zero_grad()
      outputs = net(data)
      loss = loss_func(outputs, labels)
      losses.append(loss.item())
      loss.backward()
      optimizer.step()

      if (batch_idx % 1000) == 0:
        print('epoch {} : [{}/{} ({:.0f}%)]\tLoss={:.6f}'.format(
          epoch, batch_idx*batch_size, len(loader.dataset), 
          100. * (batch_idx/len(loader)), loss.item()
        ))

    print('epoch {} : Average Loss={:.6f}\n'.format(
      epoch, np.mean(losses)
    ))

# Test routine
@torch.no_grad()
def test(net, loader, loss_func):
  test_loss = 0.0
  num_correct = 0

  for batch_idx, (data, labels) in enumerate(loader, 0):
    output = net(data)
    loss = loss_func(output, labels)
    pred = output.max(1)[1]
    num_correct += pred.eq(labels.view_as(pred)).sum().item()

  test_loss /= len(loader.dataset)
  test_accuracy = 100. * (num_correct / len(loader.dataset))

  print('Test set: Average loss={:.4f}, Accuracy: {}/{} ({:.0f}%)\n'.format(
            test_loss, num_correct, len(loader.dataset), test_accuracy
  ))

# Model
BATCH_SIZE = 32
LEARNING_RATE = 0.02
MOMENTUM = 0.9
EPOCHS = 1
net = LeNet5()
optimizer = torch.optim.SGD(
  net.parameters(), lr=LEARNING_RATE, momentum=MOMENTUM
)
loss_func = nn.CrossEntropyLoss()

# Data
transforms = torchvision.transforms.Compose([
  torchvision.transforms.Resize((32,32)),
  torchvision.transforms.ToTensor(),
])

trainset = torchvision.datasets.MNIST(
  root='./data', train=True, download=True, transform=transforms
)
trainloader = torch.utils.data.DataLoader(
  trainset, batch_size=BATCH_SIZE, shuffle=True, num_workers=2
)

testset = torchvision.datasets.MNIST(
  root='./data', train=False, download=True, transform=transforms
)
testloader = torch.utils.data.DataLoader(
  testset, batch_size=BATCH_SIZE, shuffle=False, num_workers=2
)

# Train
with torch.autograd.profiler.profile() as prof_train:
  train(net, trainloader, EPOCHS, optimizer, loss_func)

# Test
with torch.autograd.profiler.profile() as prof_test:
  test(net, testloader, loss_func)

# Log profiling data
module_name = os.path.splitext(os.path.basename(__file__))[0]
profile_log_filename = module_name + '.pytorch.profile.log'
profile_log = open(profile_log_filename, "w")
print('PyTorch profiling logged in {}\n'.format( profile_log_filename))
print(prof_train.key_averages().table(sort_by='self_cpu_time_total'), file=profile_log)
print(prof_test.key_averages().table(sort_by='self_cpu_time_total'), file=profile_log)
profile_log.close()
