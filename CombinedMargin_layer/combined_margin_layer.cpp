#include <algorithm>
#include <vector>
#include <math.h>
#include "caffe/layers/combined_margin_layer.hpp"

namespace caffe
{

template <typename Dtype>
void CombinedMarginLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype> *> &bottom, const vector<Blob<Dtype> *> &top)
{
  const CombinedMarginParameter &param = this->layer_param_.combined_margin_param();
  m1 = param.m1();
  m2 = param.m2();
  m3 = param.m3();
  transform_test_ = param.transform_test() & (this->phase_ == TRAIN);
}

template <typename Dtype>
void CombinedMarginLayer<Dtype>::Reshape(const vector<Blob<Dtype> *> &bottom,
                                         const vector<Blob<Dtype> *> &top)
{
  top[0]->ReshapeLike(*bottom[0]);
}

template <typename Dtype>
void CombinedMarginLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype> *> &bottom,
                                             const vector<Blob<Dtype> *> &top)
{
  const Dtype *bottom_data = bottom[0]->cpu_data();
  const Dtype *label_data = bottom[1]->cpu_data();
  Dtype *top_data = top[0]->mutable_cpu_data();

  int num = bottom[0]->num();
  int count = bottom[0]->count();
  int dim = count / num;

  caffe_copy(count, bottom_data, top_data);

  for (int i = 0; i < num; ++i)
  {
    int gt = static_cast<int>(label_data[i]);
    if (gt < 0)
      continue;

    Dtype cos_theta = top_data[i * dim + gt];
    float theta = acos(cos_theta);
    Dtype m1_mul_theta_plus_m2 = m1 * theta + m2;
    top_data[i * dim + gt] = cos(m1_mul_theta_plus_m2) - m3;
  }
}

template <typename Dtype>
void CombinedMarginLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype> *> &top,
                                              const vector<bool> &propagate_down,
                                              const vector<Blob<Dtype> *> &bottom)
{
  if (propagate_down[0])
  {
    const Dtype *top_diff = top[0]->cpu_diff();
    const Dtype *label_data = bottom[1]->cpu_data();
    const Dtype *bottom_data = bottom[0]->cpu_data();
    Dtype *bottom_diff = bottom[0]->mutable_cpu_diff();
    int count = bottom[0]->count();

    caffe_copy(count, top_diff, bottom_diff);

    int num = bottom[0]->num();
    int dim = count / num;
    for (int i = 0; i < num; ++i)
    {
      int gt = static_cast<int>(label_data[i]);
      if (gt < 0)
        continue;
      Dtype cos_theta = bottom_data[i * dim + gt];
      float theta = acos(cos_theta);
      Dtype m1_mul_theta_plus_m2 = m1 * theta + m2;
      Dtype diff_gt = m1 * pow(1 - pow(cos_theta, 2), -0.5) * sin(m1_mul_theta_plus_m2);
      bottom_diff[i * dim + gt] *= diff_gt;
    }
  }
}

INSTANTIATE_CLASS(CombinedMarginLayer);
REGISTER_LAYER_CLASS(CombinedMargin);

} // namespace caffe
