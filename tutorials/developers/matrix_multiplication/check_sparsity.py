from scipy.io import mmread
from scipy.sparse import csr_matrix

def calculate_sparsity(matrix):
    """Calculate the sparsity of the matrix."""
    # If matrix is not already in sparse format, convert it
    if not isinstance(matrix, csr_matrix):
        matrix = csr_matrix(matrix)
    # Calculate sparsity
    num_zeros = matrix.shape[0] * matrix.shape[1] - matrix.nnz
    sparsity = num_zeros / (matrix.shape[0] * matrix.shape[1])
    return sparsity

# Load the matrices
dense_matrix = mmread('dense_matrix.mtx')
sparse_matrix = mmread('sparse_matrix.mtx')

# Calculate and print sparsity
print("Sparsity of dense_matrix.mtx:", calculate_sparsity(dense_matrix))
print("Sparsity of sparse_matrix.mtx:", calculate_sparsity(sparse_matrix))

