#include "../nanoflann.hpp"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <opencv2/opencv.hpp>

#include <boost/filesystem.hpp>
#include <Eigen/Dense>


namespace fs = boost::filesystem;

using namespace Eigen;
using namespace std;
using namespace nanoflann;

const int SAMPLES_DIM = 15;

template <typename Der>
void generateRandomPointCloud(Eigen::MatrixBase<Der> &mat, const size_t N,
                              const size_t dim,
                              const typename Der::Scalar max_range = 10) {
  std::cout << "Generating " << N << " random points...";
  mat.resize(N, dim);
  for (size_t i = 0; i < N; i++)
    for (size_t d = 0; d < dim; d++)
      mat(i, d) = max_range * (rand() % 1000) / typename Der::Scalar(1000);
  std::cout << "done\n";
}

template <typename num_t>
void kdtree_demo(const size_t nSamples, const size_t dim) {

  Eigen::Matrix<num_t, Dynamic, Dynamic> mat(nSamples, dim);

  const num_t max_range = 20;

  // Generate points:
  generateRandomPointCloud(mat, nSamples, dim, max_range);

  //	cout << mat << endl;

  // Query point:
  std::vector<num_t> query_pt(dim);
  for (size_t d = 0; d < dim; d++)
    query_pt[d] = max_range * (rand() % 1000) / num_t(1000);

  // ------------------------------------------------------------
  // construct a kd-tree index:
  //    Some of the different possibilities (uncomment just one)
  // ------------------------------------------------------------
  // Dimensionality set at run-time (default: L2)
  //typedef KDTreeEigenMatrixAdaptor<Eigen::Matrix<num_t, Dynamic, Dynamic>>
  //    my_kd_tree_t;

  // Dimensionality set at compile-time
  //	typedef KDTreeEigenMatrixAdaptor<Eigen::Matrix<num_t, Dynamic, Dynamic>>
  // my_kd_tree_t;

  // Dimensionality set at compile-time: Explicit selection of the distance
  // metric: L2
  typedef KDTreeEigenMatrixAdaptor<
     Eigen::Matrix<num_t,Dynamic,Dynamic>, Eigen::Dynamic, nanoflann::metric_L2>  my_kd_tree_t;

  // Dimensionality set at compile-time: Explicit selection of the distance
  // metric: L2_simple
  //	typedef KDTreeEigenMatrixAdaptor<
  // Eigen::Matrix<num_t,Dynamic,Dynamic>,nanoflann::metric_L2_Simple>
  // my_kd_tree_t;

  // Dimensionality set at compile-time: Explicit selection of the distance
  // metric: L1
  //	typedef KDTreeEigenMatrixAdaptor<
  // Eigen::Matrix<num_t,Dynamic,Dynamic>,nanoflann::metric_L1>  my_kd_tree_t;

  my_kd_tree_t mat_index(dim, std::cref(mat), 10 /* max leaf */);
  mat_index.index->buildIndex();

  // do a knn search
  const size_t num_results = 3;
  vector<size_t> ret_indexes(num_results);
  vector<num_t> out_dists_sqr(num_results);

  nanoflann::KNNResultSet<num_t> resultSet(num_results);
  // resultSet.init(&ret_indexes[0], &out_dists_sqr[0]);
  resultSet.init(ret_indexes.data(), out_dists_sqr.data());

  mat_index.index->findNeighbors(resultSet, &query_pt[0],
                                 nanoflann::SearchParams(10));

  std::cout << "knnSearch(nn=" << num_results << "): \n";
  for (size_t i = 0; i < num_results; i++)
    std::cout << "ret_index[" << i << "]=" << ret_indexes[i]
              << " out_dist_sqr=" << out_dists_sqr[i] << endl;
}

void circle_demo() {
  using PC_type = Eigen::Matrix<float, Eigen::Dynamic, 2>;

  PC_type pc;
  pc.resize(100, Eigen::NoChange);

  float radius = 1.0;

  for (int i = 0; i < pc.rows(); i++)
  {
    float phi = static_cast<float>(i) / pc.rows() *2 * 3.14159265;
    pc.row(i) = radius * Eigen::Vector2f(std::cos(phi), std::sin(phi));
  }

  using my_kd_tree_t = KDTreeEigenMatrixAdaptor<
     PC_type, 2, nanoflann::metric_L2>;
    
  my_kd_tree_t my_tree(PC_type::ColsAtCompileTime, std::cref(pc));
  my_tree.index->buildIndex();

  nanoflann::KNNResultSet<float> result(1);
  size_t closest_index;
  float closest_sqdist;
  
  
  
  for (int i = 0; i < 20; i++)
  {
    float phi = static_cast<float>(i) / pc.rows() *2 * 3.14159265;
    
    Eigen::Vector2f query;
    query = radius * Eigen::Vector2f(std::cos(phi), std::sin(phi));

    result.init(&closest_index, &closest_sqdist);

    
    my_tree.index->findNeighbors(result, &query.x(), nanoflann::SearchParams());
    cout << closest_index << std::endl;
  }
}

Eigen::Matrix4d best_fit_transform(const Eigen::MatrixXd &A, const Eigen::MatrixXd &B){
    /*
    Notice:
    1/ JacobiSVD return U,S,V, S as a vector, "use U*S*Vt" to get original Matrix;
    2/ matrix type 'MatrixXd' or 'MatrixXf' matters.
    */
    Eigen::Matrix4d T = Eigen::MatrixXd::Identity(4,4);
    Eigen::Vector3d centroid_A(0,0,0);
    Eigen::Vector3d centroid_B(0,0,0);
    Eigen::MatrixXd AA = A;
    Eigen::MatrixXd BB = B;
    int row = A.rows();

    for(int i=0; i<row; i++){
        centroid_A += A.block<1,3>(i,0).transpose();
        centroid_B += B.block<1,3>(i,0).transpose();
    }
    centroid_A /= row;
    centroid_B /= row;
    for(int i=0; i<row; i++){
        AA.block<1,3>(i,0) = A.block<1,3>(i,0) - centroid_A.transpose();
        BB.block<1,3>(i,0) = B.block<1,3>(i,0) - centroid_B.transpose();
    }

    Eigen::MatrixXd H = AA.transpose()*BB;
    Eigen::MatrixXd U;
    Eigen::VectorXd S;
    Eigen::MatrixXd V;
    Eigen::MatrixXd Vt;
    Eigen::Matrix3d R;
    Eigen::Vector3d t;

    JacobiSVD<Eigen::MatrixXd> svd(H, ComputeFullU | ComputeFullV);
    U = svd.matrixU();
    S = svd.singularValues();
    V = svd.matrixV();
    Vt = V.transpose();

    R = Vt.transpose()*U.transpose();

    if (R.determinant() < 0 ){
        Vt.block<1,3>(2,0) *= -1;
        R = Vt.transpose()*U.transpose();
    }

    t = centroid_B - R*centroid_A;

    T.block<3,3>(0,0) = R;
    T.block<3,1>(0,3) = t;
    return T;

}

template <typename Der>
void Disparity2PointCloud(
        Eigen::Matrix<Der, Eigen::Dynamic, Eigen::Dynamic> &pcl, cv::Mat &disparities,
        const int &dmin, const double &baseline = 160, const double &focal_length = 3740)
    {
	int rows = disparities.rows;
	int cols = disparities.cols;
	// std::stringstream out3d;
	// out3d << output_file << ".xyz";
	// std::ofstream outfile(out3d.str());
  std::vector<std::vector<double>> points_vec;
	for (int r = 0; r < rows; ++r)
	{
		std::cout << "Reconstructing 3D point cloud from disparities... " << std::ceil(((r) / static_cast<double>(rows + 1)) * 100) << "%\r" << std::flush;
		// #pragma omp parallel for
		for (int c = 0; c < cols; ++c)
		{
			if (disparities.at<uchar>(r, c) == 0)
				continue;

			int d = (int)disparities.at<uchar>(r, c) + dmin;
			int u1 = c - cols / 2;
			int u2 = c + d - cols / 2;
			int v1 = r - rows / 2;

			const double Z = baseline * focal_length / d;
			const double X = -0.5 * (baseline * (u1 + u2)) / d;
			const double Y = baseline * v1 / d;
      std::vector<double> point_vec{X, Y, Z};
      points_vec.push_back(point_vec);
			// outfile << X << " " << Y << " " << Z << std::endl;
		}
  }
  
  int N = points_vec.size();
  pcl.resize(N, 3);
  
  for(int i = 0; i < N; i++){
    for(int j = 0; j < 3; j++){
      pcl(i, j) = points_vec[i][j];
    }
  }
}

int main(int argc, char **argv) {
  cv::Mat D1;
  cv::Mat D2;
  std::string datasetName = "Art";
  fs::path data_path("data");
  data_path = data_path / datasetName;

  fs::path d1_path = data_path / "disp1.png";
  fs::path d2_path = data_path / "disp5.png";

  D1 = cv::imread(d1_path.string(), 0);
  D2 = cv::imread(d2_path.string(), 0);
  
  Eigen::MatrixXf PCL1;
  Eigen::MatrixXf PCL2;

  Disparity2PointCloud(PCL1, D1, 200, 160, 3740);

  // cv::imshow("D1", D1);
  // cv::imshow("D2", D2);
  // int k = cv::waitKey(0); // Wait for a keystroke in the window




  // Randomize Seed
  // srand(static_cast<unsigned int>(time(nullptr)));
  // kdtree_demo<float>(1000 /* samples */, SAMPLES_DIM /* dim */);
  // circle_demo();

  return 0;
}