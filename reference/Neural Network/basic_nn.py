"""
Example for Neural Network
(dataset: make_moons)
"""

from sklearn import datasets, model_selection
from matplotlib import pyplot
import numpy





class Layer:
    """ Build a layer of a neural network """
    def __init__(self, n_input, n_neurons, activation=None, weights=None, bias=None):
        """
        Args:
            n_input (int): number of input node in the layer
            n_neurons (int): number of output node in the layer
            activation (str, optional): type of activation formula
            weights (array float, optional): _description_. Defaults to generate in model.
            bias (array float, optional): _description_. Defaults to generate in model.
        """
        self.weights = weights if weights is not None else numpy.random.randn(n_input, n_neurons) * numpy.sqrt(1 / n_neurons)
        self.bias = bias if bias is not None else numpy.random.rand(n_neurons) * 0.1
        self.activation = activation        # e.g. sigmoid
        self.last_activation = None         # output O
        self.error = None                   # for compute delta variable of current layer
        self.delta = None                   # for record delta variable of current layer, for gradient decend
        
    def activate(self, x):
        # Forward propagation ( X @ W + b )
        r = numpy.dot(x, self.weights) + self.bias
        self.last_activation = self._apply_activation(r)
        return self.last_activation
        
    def _apply_activation(self, r):
        # Compute output of activation model
        
        if self.activation is None:
            # no activation, return original value
            return r
        
        elif self.activation == 'relu':
            # ReLU activation
            return numpy.maximum(r, 0)
        
        elif self.activation == 'tanh':
            # tanh activation
            return numpy.tanh(r)
        
        elif self.activation == 'sigmoid':
            # sigmoid activation
            return 1 / (1 + numpy.exp(-r))
        
        return r
    
    def apply_activation_derivative(self, r):
        # Compute derivative of activation model
        
        if self.activation is None:
            # no activation
            return numpy.ones_like(r)
        
        elif self.activation == 'relu':
            # ReLU activation
            grad = numpy.array(r, copy=True)
            grad[r > 0] = 1.
            grad[r <= 0] = 0.
            return grad
        
        elif self.activation == 'tanh':
            # tanh activation
            return 1 - r ** 2
        
        elif self.activation == 'sigmoid':
            # sigmoid activation
            return r * (1 - r)
        
        return r
    
    
class NeuralNetwork:
    """ Build a neural network model """
    def __init__(self):
        self._layers = []       # list of layers
    
    def add_layer(self, layer):
        self._layers.append(layer)
        
    def feed_forward(self, X):
        # forward propagation
        for layer in self._layers:
            X = layer.activate(X)
            
        return X
    
    def backpropagation(self, X, y, learning_rate):
        # backpropagation
        
        # first do forward prograpation to get output value
        output = self.feed_forward(X)
        
        # second compute the loss and its derivative for each layer from back to front
        for i in reversed(range(len(self._layers))):
            # go from the last layer to the first layer
            layer = self._layers[i]
            
            if layer == self._layers[-1]:
                # Output Layer (the last layer)
                layer.error = y - output
                layer.delta = layer.error * layer.apply_activation_derivative(output)
                
            else:
                # Hidden layers
                next_layer = self._layers[i + 1]
                layer.error = numpy.dot(next_layer.weights, next_layer.delta)
                layer.delta = layer.error * layer.apply_activation_derivative(layer.last_activation)
            
        # third update weights and biases for each layer from front to back
        for i in range(len(self._layers)):
            # go from the first layer to the last layer
            layer = self._layers[i]
            
            # get the output of previous layer
            o_i = numpy.atleast_2d(X if i == 0 else self._layers[i - 1].last_activation)
            # gradient descend
            layer.weights += layer.delta * o_i.T * learning_rate
            
    def train(self, X_train, X_test, y_train, y_test, learning_rate, max_epochs):
        # one-hot encoding the true class labels
        y_onehot = numpy.zeros((y_train.shape[0], N_LABEL))
        y_onehot[numpy.arange(y_train.shape[0]), y_train] = 1
        
        mses = []
        for i in range(max_epochs):
            # repeat for epochs
            
            for j in range(len(X_train)):
                # in each epochs working per sample
                self.backpropagation(X_train[j], y_onehot[j], learning_rate)
                
            if i % 10 == 0:
                # regularly print MSE loss
                mse = numpy.mean(numpy.square(y_onehot - self.feed_forward(X_train)))
                mses.append(mse)
                print(f'Epoch: #{i}, MSE: {float(mse)}')
                
                # print(f'Accuracy: {self.accuracy(self.predict(X_test), y_test.flatten()) * 100}')
    
    
# show dataset distribution (X as 2D coordinate, y as label)
def make_plot(X, y, plot_name, file_name=None, XX=None, YY=None, preds=None):
    pyplot.figure(figsize=(16, 12))
    
    axes = pyplot.gca()
    axes.set(xlabel='$x_1$', ylabel='$x_2$')
    
    pyplot.title(plot_name, fontsize=30)
    pyplot.subplots_adjust(left=0.20)
    pyplot.subplots_adjust(right=0.80)
    
    if XX is not None and YY is not None and preds is not None:
        pyplot.contourf(XX, YY, preds.reshape(XX.shape), 25, alpha=1)
        pyplot.contour(XX, YY, preds.reshape(XX.shape), levels=[.5], vmin=0, vmax=.6)
    
    pyplot.scatter(X[:, 0], X[:, 1], c=y.ravel(), s=40, edgecolors='none')
    
    
TEST_SIZE = 0.3         # validation ratio
N_SAMPLE = 2000         # no of sampling point
N_FEATURE = 2           # no of input features
N_LABEL = 2             # no of output class labels

# build dataset from scikit learn toolbox
# selected "make_moons" dataset has 2 features and 2 class labels
X, y = datasets.make_moons(n_samples=N_SAMPLE, noise=0.2, random_state=100)
# split dataset into train set and test set in ratio
X_train, X_test, y_train, y_test = model_selection.train_test_split(X, y, test_size=TEST_SIZE, random_state=42)
print(X.shape, y.shape)

# Construct a Neural Network (3-layers)
nn = NeuralNetwork()
nn.add_layer(Layer(N_FEATURE, 25, 'sigmoid'))
nn.add_layer(Layer(25, 50, 'sigmoid'))
nn.add_layer(Layer(50, 25, 'sigmoid'))
nn.add_layer(Layer(25, N_LABEL, 'sigmoid'))
nn.train(X_train, X_test, y_train, y_test, learning_rate=0.01, max_epochs=1000)

# Prepare Mesh Grid for drawing Contour graph
XXX_axe = numpy.arange(-1.5, 2.5, 0.01)
YYY_axe = numpy.arange(-1.0, 1.5, 0.01)
XXX, YYY = numpy.meshgrid(XXX_axe, YYY_axe)
# Flatten meshgrid to prepare feeding input to model
XXX_ = numpy.array(XXX).reshape(250*400)
YYY_ = numpy.array(YYY).reshape(250*400)
ZZZ_ = numpy.stack((XXX_, YYY_), axis=1)
# Forward feeding flatten input to model
prediction = nn.feed_forward(ZZZ_)
# Prepare mesh grid form of the predication
ZZZ = prediction[:, 0].reshape((250, 400))

# Plot the contour graph with the predication
make_plot(X, y, 'Classification Dataset Visualization', XX=XXX, YY=YYY, preds=ZZZ)
pyplot.show()
