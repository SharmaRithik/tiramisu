import numpy as np
from scipy.sparse import random
from scipy.io import mmwrite

# Setting a random seed for reproducibility
np.random.seed(42)

# Create a dense matrix with random values between 0 and 1
dense_matrix = np.random.rand(1000, 1000)

# Create a "sparse" matrix with 0.01 density (1% non-zero elements). This will still be a full 2D array.
sparse_matrix_data = random(1000, 1000, density=0.01).toarray()

# Save the dense matrix
mmwrite('dense_matrix.mtx', dense_matrix)

# Save the "sparse" matrix
mmwrite('sparse_matrix.mtx', sparse_matrix_data)

