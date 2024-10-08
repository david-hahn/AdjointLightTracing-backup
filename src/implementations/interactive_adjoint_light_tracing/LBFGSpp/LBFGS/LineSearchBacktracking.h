// Copyright (C) 2016-2019 Yixuan Qiu <yixuan.qiu@cos.name>
// Under MIT license

#ifndef LINE_SEARCH_BACKTRACKING_H
#define LINE_SEARCH_BACKTRACKING_H

#include <Eigen/Core>
#include <stdexcept>  // std::runtime_error


namespace LBFGSpp {


///
/// The backtracking line search algorithm for LBFGS. Mainly for internal use.
///
template <typename Scalar>
class LineSearchBacktracking
{
private:
    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> Vector;

public:
    ///
    /// Line search by backtracking.
    ///
    /// \param f      A function object such that `f(x, grad)` returns the
    ///               objective function value at `x`, and overwrites `grad` with
    ///               the gradient.
    /// \param fx     In: The objective function value at the current point.
    ///               Out: The function value at the new point.
    /// \param x      Out: The new point moved to.
    /// \param grad   In: The current gradient vector. Out: The gradient at the
    ///               new point.
    /// \param step   In: The initial step length. Out: The calculated step length.
    /// \param drt    The current moving direction.
    /// \param xp     The current point.
    /// \param param  Parameters for the LBFGS algorithm
    ///
    template <typename Foo>
    static void LineSearch(Foo& f, Scalar& fx, Vector& x, Vector& grad,
                           Scalar& step,
                           const Vector& drt, const Vector& xp,
                           const LBFGSParam<Scalar>& param)
    {
        // Decreasing and increasing factors
        const Scalar dec = 0.5;
        const Scalar inc = 2.1;

        // Check the value of step
        if(step <= Scalar(0))
            std::invalid_argument("'step' must be positive");

        // Save the function value at the current x
        const Scalar fx_init = fx;
        // Projection of gradient on the search direction
        const Scalar dg_init = grad.dot(drt);
        // Make sure d points to a descent direction
        if(dg_init > 0)
            std::logic_error("the moving direction increases the objective function value");

        const Scalar dg_test = param.ftol * dg_init;
        Scalar width;

		Scalar bestF = fx, bestStep = step; Vector bestGrad = grad, bestX = x;

        int iter;
        for(iter = 0; iter < param.max_linesearch; iter++)
        {	fprintf(stderr,"\n%% LS step %6.2lg ", step);
            // x_{k+1} = x_k + step * d_k
            x.noalias() = xp + step * drt;
            // Evaluate this candidate
            fx = f(x, grad);

			if( fx < bestF ){ bestF = fx; bestStep = step; bestGrad = grad; bestX = x;}

            if(fx > fx_init + step * dg_test)
            {
                width = dec;
            } else {
                // Armijo condition is met
                if(param.linesearch == LBFGS_LINESEARCH_BACKTRACKING_ARMIJO)
                    break;

                const Scalar dg = grad.dot(drt);
                if(dg < param.wolfe * dg_init)
                {
                    width = inc;
                } else {
                    // Regular Wolfe condition is met
                    if(param.linesearch == LBFGS_LINESEARCH_BACKTRACKING_WOLFE)
                        break;

                    if(dg > -param.wolfe * dg_init)
                    {
                        width = dec;
                    } else {
                        // Strong Wolfe condition is met
                        break;
                    }
                }
            }

            if(step < param.min_step)
                iter = param.max_linesearch; // breaks the loop //throw std::runtime_error("the line search step became smaller than the minimum value allowed");

            if(step > param.max_step)
                iter = param.max_linesearch; // breaks the loop //throw std::runtime_error("the line search step became larger than the maximum value allowed");

            step *= width;
        }
		if( iter >= param.max_linesearch && fx > bestF ){ fprintf(stderr,"\n%% LS OUT OF ITERS - REPLACING WITH BEST FOUND VALUE!\n"); fx = bestF; step = bestStep; grad = bestGrad; x = bestX;}
    }
};


} // namespace LBFGSpp

#endif // LINE_SEARCH_BACKTRACKING_H
