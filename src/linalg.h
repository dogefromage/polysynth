#include <stdio.h>
#include <stdlib.h>

// Solve a 3x3 linear system using Gaussian elimination
void solve_3x3(float A[3][3], float b[3], float x[3]) {
    // Forward elimination
    for (int i = 0; i < 3; ++i) {
        // Normalize the pivot row
        float diag = A[i][i];
        for (int j = i; j < 3; ++j) {
            A[i][j] /= diag;
        }
        b[i] /= diag;

        // Eliminate the current column from subsequent rows
        for (int k = i + 1; k < 3; ++k) {
            float factor = A[k][i];
            for (int j = i; j < 3; ++j) {
                A[k][j] -= factor * A[i][j];
            }
            b[k] -= factor * b[i];
        }
    }

    // Back substitution
    for (int i = 2; i >= 0; --i) {
        x[i] = b[i];
        for (int j = i + 1; j < 3; ++j) {
            x[i] -= A[i][j] * x[j];
        }
    }
}

// Fit a parabola y = a + bx + cx^2 using least squares
void fit_parabola(float* x, float* y, int m, float* coefficients) {
    // Initialize sums for A^T A and A^T b
    float S[5] = {0};     // Sums: S0 = sum(1), S1 = sum(x), ..., S4 = sum(x^4)
    float T[3] = {0};     // T: T0 = sum(y), T1 = sum(x*y), T2 = sum(x^2*y)

    // Compute sums iteratively
    for (int i = 0; i < m; ++i) {
        float xi = x[i];
        float yi = y[i];
        float xi2 = xi * xi;

        S[0] += 1;
        S[1] += xi;
        S[2] += xi2;
        S[3] += xi2 * xi;
        S[4] += xi2 * xi2;

        T[0] += yi;
        T[1] += xi * yi;
        T[2] += xi2 * yi;
    }

    // Form A^T A matrix and A^T b vector
    float ATA[3][3] = {
        {S[0], S[1], S[2]},
        {S[1], S[2], S[3]},
        {S[2], S[3], S[4]}
    };
    float ATb[3] = {T[0], T[1], T[2]};

    // Solve A^T A * x = A^T b
    solve_3x3(ATA, ATb, coefficients);
}

// int main() {
//     // Example data: y = 2 + 3x + 4x^2 with some noise
//     int m = 5;
//     float x[] = {1, 2, 3, 4, 5};
//     float y[] = {9, 22, 43, 72, 109}; // Observed y values
//     float coefficients[3];

//     fit_parabola(x, y, m, coefficients);

//     printf("Fitted coefficients: a = %.2f, b = %.2f, c = %.2f\n",
//            coefficients[0], coefficients[1], coefficients[2]);

//     return 0;
// }
