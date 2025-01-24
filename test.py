import numpy
import pandas
import tensorflow

from matplotlib import pyplot
from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import mean_squared_error, mean_absolute_error

from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import LSTM, Dense, Dropout


url = 'https://raw.githubusercontent.com/jbrownlee/Datasets/master/daily-min-temperatures.csv'
df = pandas.read_csv(url, parse_dates=['Date'], index_col='Date')

print(df.head())

# df.plot()
# pyplot.show()

# Initialize the scaler
scaler = MinMaxScaler(feature_range=(0, 1))

# Fit and transform the data
scaled_data = scaler.fit_transform(df)

# Convert to a DataFrame for easier handling
scaled_df = pandas.DataFrame(scaled_data, index=df.index, columns=df.columns)

print(df)
print(scaled_data)

def create_sequences(data, seq_length):
    X = []
    y = []
    for i in range(len(data) - seq_length):
        X.append(data[i:i+seq_length, 0])
        y.append(data[i+seq_length, 0])
    return numpy.array(X), numpy.array(y)


sequence_length = 60  # You can adjust this based on your data

X, y = create_sequences(scaled_data, sequence_length)

# Reshape X to be 3D as LSTM expects [samples, timesteps, features]
X = X.reshape((X.shape[0], X.shape[1], 1))

print(f'Training samples: {X.shape[0]}')
print(f'Timesteps: {X.shape[1]}')

# Define the split ratio
train_size = int(len(X) * 0.8)
test_size = len(X) - train_size

# Split the data
X_train, X_test = X[:train_size], X[train_size:]
y_train, y_test = y[:train_size], y[train_size:]

print(f'Training samples: {X_train.shape[0]}')
print(f'Testing samples: {X_test.shape[0]}')

model = Sequential()

# First LSTM layer with Dropout
model.add(LSTM(50, return_sequences=True, input_shape=(sequence_length, 1)))
model.add(Dropout(0.2))

# Second LSTM layer with Dropout
model.add(LSTM(50, return_sequences=False))
model.add(Dropout(0.2))

# Output layer
model.add(Dense(1))

# Display the model's architecture
model.summary()

# Compile the model
model.compile(optimizer='adam', loss='mean_squared_error')

# Train the model
history = model.fit(
    X_train, y_train,
    epochs=50,
    batch_size=32,
    validation_split=0.1,  # 10% of training data for validation
    verbose=1
)

# Predict on the test set
y_pred = model.predict(X_test)

# Inverse transform the scaled data back to original scale
y_pred_inv = scaler.inverse_transform(y_pred)
y_test_inv = scaler.inverse_transform(y_test.reshape(-1, 1))

# Calculate evaluation metrics
mse = mean_squared_error(y_test_inv, y_pred_inv)
mae = mean_absolute_error(y_test_inv, y_pred_inv)

print(f'Mean Squared Error (MSE): {mse}')
print(f'Mean Absolute Error (MAE): {mae}')

# Plot the results
pyplot.figure(figsize=(14,5))
pyplot.plot(df.index[-len(y_test_inv):], y_test_inv, label='Actual Temperature')
pyplot.plot(df.index[-len(y_pred_inv):], y_pred_inv, label='Predicted Temperature')
pyplot.xlabel('Date')
pyplot.ylabel('Temperature')
pyplot.title('LSTM Model Predictions vs Actual Values')
pyplot.legend()
pyplot.show()
