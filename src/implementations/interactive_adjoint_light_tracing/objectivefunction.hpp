#pragma once

#include <Eigen/Eigen>
#include <vector>

class ObjectiveFunction {
public:
						ObjectiveFunction() = default;
	virtual				~ObjectiveFunction() = default;
	virtual float		operator()(Eigen::VectorXf& aX, Eigen::VectorXf& aDx) {
							aDx.setZero();
							constexpr float phi = 0.0f;
							return phi;
						}
protected:
	static float		weightedResidualNormAndDerivative(const Eigen::Ref<Eigen::VectorXf>& aVertexWeights, const Eigen::Ref<Eigen::VectorXf>& aVertexAreas,
							const Eigen::Ref<Eigen::VectorXf>& aTarget, const Eigen::Ref<Eigen::VectorXf>& aX, Eigen::Ref<Eigen::VectorXf> aDx);
};

class SimpleObjectiveFunction final : public ObjectiveFunction {
public:
						
						SimpleObjectiveFunction(const Eigen::Ref<Eigen::VectorXf>& aVertexWeights, const Eigen::Ref<Eigen::VectorXf>& aVertexAreas, 
							const Eigen::Matrix<float, -1, -1, Eigen::RowMajor>& aTarget) : mTarget(aTarget), mVertexWeights(aVertexWeights), mVertexAreas(aVertexAreas) {}

	float				operator()(Eigen::VectorXf& aX, Eigen::VectorXf& aDx) override;

	Eigen::VectorXf		mTarget;
	Eigen::VectorXf		mVertexWeights;
	Eigen::VectorXf		mVertexAreas;
};



class MultiChannelObjectiveFunction final : public ObjectiveFunction {
public:
						
						
						
						
						
						
						MultiChannelObjectiveFunction(const Eigen::Ref<Eigen::VectorXf>& aVertexWeights, const Eigen::Ref<Eigen::VectorXf>& aVertexAreas, 
							const Eigen::Ref<Eigen::VectorXf>& aChannelWeights, const Eigen::Ref<Eigen::VectorXf>& aVertexColor, const Eigen::Matrix<float, -1, -1, Eigen::RowMajor>& aXTarget) :
								mTarget(aXTarget), mVertexWeights(aVertexWeights), mVertexAreas(aVertexAreas), mChannelWeights(aChannelWeights), mVertexColor(aVertexColor) {}

	float				operator()(Eigen::VectorXf& aX, Eigen::VectorXf& aDx) override;

	Eigen::MatrixXf		mTarget;
	Eigen::VectorXf		mVertexWeights;
	Eigen::VectorXf		mVertexAreas;
	Eigen::VectorXf		mVertexColor;
	Eigen::VectorXf		mChannelWeights; 
};


class ConsistentMassMultiChannelObjectiveFunction final : public ObjectiveFunction {
public:
						
						
						
						
						
						
						
						ConsistentMassMultiChannelObjectiveFunction(const Eigen::Ref<Eigen::VectorXf>& aVertexWeights, Eigen::MatrixXi& elems, Eigen::MatrixXf& coords,
							const Eigen::Ref<Eigen::VectorXf>& aChannelWeights, const Eigen::Ref<Eigen::VectorXf>& aVertexColor, const Eigen::Matrix<float, -1, -1, Eigen::RowMajor>& aXTarget) :
								mTarget(aXTarget), mChannelWeights(aChannelWeights), mVertexColor(aVertexColor) {
							buildConsistentMassMatrix(elems, coords, aVertexWeights);
						}

	float				operator()(Eigen::VectorXf& aX, Eigen::VectorXf& aDx) override;
	void buildConsistentMassMatrix(Eigen::MatrixXi& elems, Eigen::MatrixXf& coords, const Eigen::Ref<Eigen::VectorXf>& aVertexWeights);

	Eigen::MatrixXf		mTarget;
	Eigen::SparseMatrix<float> mM;
	Eigen::VectorXf		mVertexColor;
	Eigen::VectorXf		mChannelWeights; 
};
