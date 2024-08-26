// Copyright (C) 2016-2019 Yixuan Qiu <yixuan.qiu@cos.name>
// Under MIT license

#ifndef LBFGS_H
#define LBFGS_H

#include <Eigen/Core>
#include <iostream>
#include <deque>
#include "LBFGS/Param.h"
#include "LBFGS/LineSearchBacktracking.h"
#include "LBFGS/LineSearchBracketing.h"


namespace LBFGSpp {

    using namespace std;
///
/// LBFGS solver for unconstrained numerical optimization
///
template < typename Scalar,
           template<class> class LineSearch = LineSearchBacktracking >
class LBFGSSolver
{
private:
    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> Vector;
    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> Matrix;
    typedef Eigen::Map<Vector> MapVec;

    const LBFGSParam<Scalar>& m_param;  // Parameters to control the LBFGS algorithm
    //Matrix                    m_s;      // History of the s vectors
    //Matrix                    m_y;      // History of the y vectors
    //Vector                    m_ys;     // History of the s'y values
    //Vector                    m_alpha;  // History of the step lengths
    Vector                    m_fx;     // History of the objective function values
    Vector                    m_xp;     // Old x
    Vector                    m_grad;   // New gradient
    Vector                    m_gradp;  // Old gradient
    Vector                    m_drt;    // Moving direction

    std::deque<Vector>        m_s_hist;      // History of the s vectors
    std::deque<Vector>        m_y_hist;      // History of the y vectors
    std::deque<Scalar>        m_alpha_hist;  // History of the step lengths

    inline void reset(int n)
    {
        const int m = m_param.m;
        //m_s.resize(n, m);
        //m_y.resize(n, m);
        //m_ys.resize(m);
        //m_alpha.resize(m);
        m_xp.resize(n);
        m_grad.resize(n);
        m_gradp.resize(n);
        m_drt.resize(n);
        if(m_param.past > 0)
            m_fx.resize(m_param.past);
    }

public:
    ///
    /// Constructor for LBFGS solver.
    ///
    /// \param param An object of \ref LBFGSParam to store parameters for the
    ///        algorithm
    ///
    LBFGSSolver(const LBFGSParam<Scalar>& param) :
        m_param(param)
    {
        m_param.check_param();
    }

    ///
    /// Minimizing a multivariate function using LBFGS algorithm.
    /// Exceptions will be thrown if error occurs.
    ///
    /// \param f  A function object such that `f(x, grad)` returns the
    ///           objective function value at `x`, and overwrites `grad` with
    ///           the gradient.
    /// \param x  In: An initial guess of the optimal point. Out: The best point
    ///           found.
    /// \param fx Out: The objective function value at `x`.
    ///
    /// \return Number of iterations used.
    ///
    template <typename Foo>
    inline int minimize(Foo& f, Vector& x, Scalar& fx)
    {
        const int n = x.size();
        const int fpast = m_param.past;
        reset(n);

        // Evaluate function and compute gradient
        fx = f(x, m_grad);
        Scalar xnorm = x.norm();
        Scalar gnorm = m_grad.norm();
        if(fpast > 0)
            m_fx[0] = fx;

        // Early exit if the initial x is already a minimizer
        if(gnorm <= m_param.epsilon * std::max(xnorm, Scalar(1.0)))
        {
            return 1;
        }

        // Initial direction
        m_drt = -m_grad;
        // Initial step
        Scalar step = m_param.init_step / m_drt.norm();

        int k = 1;
        int end = 0;
        int bound = 0; //std::min(m_param.m, k);
        for( ; ; )
        {
            // Save the curent x and gradient
            m_xp = x;
            m_gradp = m_grad;

            // Line search to update x, fx and gradient
			fprintf(stderr,"\n%% new LS dir. (d'g=%.2lg) ", step*m_drt.dot(m_grad)); cerr << m_drt.block(0,0,m_drt.size()>5?5:m_drt.size(),1).transpose(); if( m_drt.size()>5 ) fprintf(stderr," ... ");
            LineSearch<Scalar>::LineSearch(f, fx, x, m_grad, step, m_drt, m_xp, m_param);

            // New x norm and gradient norm
            xnorm = x.norm();
            gnorm = m_grad.norm();

            // Convergence test -- gradient
            if(gnorm <= m_param.epsilon * std::max(xnorm, Scalar(1.0)))
            {
                return k;
            }
            // Convergence test -- objective function value
            if(fpast > 0)
            {
                if(k >= fpast && ((m_fx[k % fpast] - fx) / fx) < m_param.delta) //if(k >= fpast && std::abs((m_fx[k % fpast] - fx) / fx) < m_param.delta)
                    return k;

                m_fx[k % fpast] = fx;
            }
            // Maximum number of iterations
            if(m_param.max_iterations != 0 && k >= m_param.max_iterations)
            {
                return k;
            }


            // ys = y's
            // yy = y'y
            Scalar ys = (x - m_xp).dot(m_grad - m_gradp);
            Scalar yy = (m_grad - m_gradp).squaredNorm();

            if( ys > m_param.ftol ){ // only store the update vectors if y's is sufficiently positive
                //// Update s and y
                //// s_{k+1} = x_{k+1} - x_k
                //// y_{k+1} = g_{k+1} - g_k
                //MapVec svec(&m_s(0, end), n);
                //MapVec yvec(&m_y(0, end), n);
                //svec.noalias() = x - m_xp;
                //yvec.noalias() = m_grad - m_gradp;
                //m_ys[end] = (x - m_xp).dot(m_grad - m_gradp);

                //bound = std::min(m_param.m, bound+1);
                //end = (end + 1) % m_param.m;
                ////fprintf(stderr,"\n%% STORE ...");
                m_s_hist.push_back(x - m_xp);
                m_y_hist.push_back(m_grad - m_gradp);
            }else{
                if( m_param.removeNewestOnCurvatureFailure && m_s_hist.size() > 0 ){ // if failed to find a good step, remove the latest added history entry - maybe it was to blame for the bad step?
                    m_s_hist.pop_back();
                    m_y_hist.pop_back();
                }
                if( m_param.removeOldestOnCurvatureFailure && m_s_hist.size() > 0 ){ // also remove the oldest - who knows which one is really to blame?
                    m_s_hist.pop_front();
                    m_y_hist.pop_front();
                }
            }

            fprintf(stderr,"\n%% L-BFGS mem size %d of %d ", (int)(m_s_hist.size()), m_param.m);

            // Recursive formula to compute d = -H * g
            m_drt = -m_grad;
            //int j = end;
            //for(int i = 0; i < bound; i++)
            //{
            //    j = (j + m_param.m - 1) % m_param.m;
            //    MapVec sj(&m_s(0, j), n);
            //    MapVec yj(&m_y(0, j), n);
            //    m_alpha[j] = sj.dot(m_drt) / m_ys[j];
            //    m_drt.noalias() -= m_alpha[j] * yj;
            //}
            std::deque<Scalar> alpha;
            {   // first loop goes new-to-old (back-to-front) through history
                typename std::deque<Vector>::reverse_iterator y_it = m_y_hist.rbegin();
                for(typename std::deque<Vector>::reverse_iterator s_it = m_s_hist.rbegin(); s_it != m_s_hist.rend(); ++s_it, ++y_it){
                    Scalar alpha_j =  s_it->dot(m_drt) / s_it->dot(*y_it) ;
                    alpha.push_front(alpha_j);
                    m_drt -= alpha_j * (*y_it);
                }
            }

            m_drt *= (ys / yy);

            //for(int i = 0; i < bound; i++)
            //{
            //    MapVec sj(&m_s(0, j), n);
            //    MapVec yj(&m_y(0, j), n);
            //    Scalar beta = yj.dot(m_drt) / m_ys[j];
            //    m_drt.noalias() += (m_alpha[j] - beta) * sj;
            //    j = (j + 1) % m_param.m;
            //}
            {   // second loop goes old-to-new (front-to-back)
                typename std::deque<Vector>::iterator y_it = m_y_hist.begin();
                for(typename std::deque<Vector>::iterator s_it = m_s_hist.begin(); s_it != m_s_hist.end(); ++s_it, ++y_it){
                    Scalar beta_j = ( y_it->dot(m_drt) / s_it->dot(*y_it) );
                    m_drt += (alpha.front() - beta_j) * (*s_it);
                    alpha.pop_front();
                }
            }

            if( m_s_hist.size()>=m_param.m ){ // remove old entries at size limit
                m_s_hist.pop_front();
                m_y_hist.pop_front();
            }

            // step = 1.0 as initial guess
            step = Scalar(1.0);
            k++;
        }

        return k;
    }
};


} // namespace LBFGSpp

#endif // LBFGS_H
