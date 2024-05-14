#include "PBHs_cal.h" //PBHs计算头文件
#include <stdlib.h>
#include <string.h>

//基本操作

//arb_printn(AAA, 50,0);printf("\n"); //打印变量
//arb_init(AAA); //初始化
//arb_set_str(AAA,"3.14E10",100); //设定值
//arb_clear(AAA); //释放
//错误、失败等可用 flint_abort(); 表示
//对于需要传递指针的函数，不需要传递时，以 NULL 增写指针参数


int main(int argc, char* argv[]) //参数数目argc，参数 argv[i]
{
    //基本量设定
    slong prec;
    prec=500; //控制计算精度， 500，精度在 140 位左右；300，在80位左右，64，在15位左右
    
    
    Stdout_verbose=false; //命令行输出详细模式，true/false
    
    
    //输出文件配制
    get_save_path(Path_save);
    strcat(Out_date_file, "date/date.txt"); //字符串添加，数据输出文件
    strcat(Out_fitted_file, "date/fit.txt"); //字符串添加，拟合数据输出文件
    strcat(Out_picture_file, "draw.txt"); //字符串添加，画图数据输出文件
    
    
    printf("\ncomputation start...\n");
    
    
    Set_global_variable(prec); //初始化各个全局变量
    
    
    Set_cal_math(prec);//常数和数学计算设定
    
    
    //
    //测试各功能、各函数是否正常
    //
    routine_test(prec); exit(0); //测试程序
    
    
    Set_physical_parameter(prec); //宇宙学基本参数设定
    
    
    /*
    //从命令行中读取相关参数
    arb_t input_sigma,input_k_star;
    arb_init(input_sigma);
    arb_init(input_k_star);
    
    //arb_set_str(input_sigma,argv[1],prec);
    arb_set_str(input_k_star,argv[1],prec);
    
    printf("command intput arg 1: %s\n",argv[1]);
    arb_printn(input_sigma, 50,0);printf("\n");
    //exit(0);
    */
    
    //功率谱相关设定
    //功率谱类型 delta_type/lognormal_type/power_law_type/box_type/broken_power_law_type/link_cmb_type
    Power_spectrum_type=lognormal_type;
    
    
    Set_power_spectra(argv[1],prec); //功率谱相关具体参数设定，可由命令行传递参数
    
    
    //是否考虑转移函数的影响，需要求协方差矩阵之前，若在ζ(r)参数化时要考虑转移函数，则需要求(σ_n)^2等之前
    //转移函数，对于各协方差矩阵元几乎有一半的影响，均减小了一半
    Transfer_Function=true;
    
    
    Set_zeta_r_cal(prec); //ζ(r)相关计算求解，包括求解其相关的辅助变量
    
    
    //主要计算设定
    //包括：非高斯性参数，各种功率谱和各种非高斯性情况下 r_m，μ_th，β，f等计算参数的设定，临界坍缩等
    Set_main_cal(argv[1], prec); //可从命令行读取参数
    
    
    //变量Mu_2_th， ζ(r) 的参数 µ 的临界值 
    // Mu_2_th 与 K_3_square 有关，不过在简化的情况下，Mu_2_th 已独立出来，只需要求解一次
    //变量Q_parameter_th，ζ(r)取临界值时的q参数
    
    //求临界值的四种方式 q_parameter_method_simple / q_parameter_method_new
    //                average_method_simple / average_method_new
    Mu_2_TH_METHOD=q_parameter_method_new;
    
    
    //寻找阈值
    Find_Mu_2_th(Mu_2_th, prec);
    //arb_set_str(Mu_2_th,"0.4",prec);
    //arb_printn(Mu_2_th, 60,0);printf("\n\n\n");
    //exit(0);
    
    
    //
    //PS相关最初设定
    //
    //在PS计算协方差矩阵元时，需要 r_m 的位置
    //我们在这里，统一设成 μ=μ_th 时对应的位置
    
    arb_set(Mu_2, Mu_2_th); //算 r_m 时，设 Mu_2 为其临界值
    //arb_set_str(Mu_2,"0.615327",prec);
    Find_r_max(R_MAX, prec); //计算 r_m，需在 Mu_2 设定后，因为 r_m 与 Mu_2 有关
                             //且要在计算方差前，计算方差要用r_m
    //arb_set_str(R_MAX,"1.7e-13",prec);
    printf("r_max： ");arb_printn(R_MAX, 60,0);printf("\n");
    //exit(0);
    
    
    //对于δ谱，r_m*k_*为一定值
    arb_mul(R_m_times_K, R_MAX, K_star,prec);
    printf("R_m_times_K： ");arb_printn(R_m_times_K, 60,0);printf("\n\n");
    
    
    
    //PS 计算方差 XX YY XY 的值
    Variance_XX(PS_Sigma_XX,R_MAX,prec);
    Variance_XY(PS_Sigma_XY,R_MAX,prec);
    arb_set(PS_Sigma_YX,PS_Sigma_XY);
    Variance_YY(PS_Sigma_YY,R_MAX,prec);
    
    
    arb_set(PS_Sigma_XX_save,PS_Sigma_XX);//协方差保存备用
    arb_set(PS_Sigma_XY_save,PS_Sigma_XY);
    arb_set(PS_Sigma_YY_save,PS_Sigma_YY);
    
    
    printf("Sigma_XX: ");arb_printn(PS_Sigma_XX, 50,0);printf("\n");
    printf("Sigma_XY: ");arb_printn(PS_Sigma_XY, 50,0);printf("\n");
    printf("Sigma_YY: ");arb_printn(PS_Sigma_YY, 50,0);printf("\n\n\n");
    //exit(0);
    
    
    // PS 计算 C_th
    //在计算此之前，需求出 Mu_2_th 和 R_MAX，并设 Mu_2=Mu_2_th
    Q_parameter(Q_parameter_th,R_MAX,prec); 
    
    printf("q parameter_th: ");arb_printn(Q_parameter_th, 50,0);printf("\n\n");
    
    if(Mu_2_TH_METHOD==q_parameter_method_new)
    {
        Delta_c_q_parameter_new(PS_C_th,Q_parameter_th,prec);
    }else if(Mu_2_TH_METHOD==q_parameter_method_simple)
    {
        Delta_c_q_parameter_simple(PS_C_th,Q_parameter_th,prec);
    }else
    {
        C_r_profile_n(PS_C_th,R_MAX,0,prec); 
    }
    //arb_set_str(PS_C_th,"0.5869",prec);
    printf("\nFind C_th：");arb_printn(PS_C_th, 60,0);printf("\n\n\n");
    //exit(0);
    
    //求线性compaction function C_l 的threshold
    Trans_C_to_C_l(PS_C_l_th,PS_C_th,prec);
    
    
    //PS 计算 M_ratio_max 
    //M/M_H=K*[(C_l-3/8*C_l^2)-C_th]^γ  
    //其中 C_l 的最大取值为 4/3， 也对应了 M 的最大取值，即 M/M_H 的最大取值
    //这里打印出 M/M_H 的最大取值，方便后面 M/M_H 范围的选取，同时也作为后面计算范围的判据
    
    arb_t t_cl; //中间借用临时变量
    arb_init(t_cl);
    arb_one(t_cl); // t_cl = 4/3
    arb_mul_ui(t_cl,t_cl,4,prec);
    arb_div_ui(t_cl,t_cl,3,prec); // t_cl = 4/3 ，对应于两类型PBH的分界点
    PS_C_l_to_M_ratio(PS_M_ratio_max,t_cl,prec);
    
    
    //注意，这里M/M_{H}不能精确的取到最大值，会有 P(C_l->4/3) --> P(C) 的发散问题
    //但P(C)的积分没有发散问题，为了数值计算上的方便，做一点微小的截断
    arb_set_str(t_cl, "1E-80", prec); // C_l_max=C_l_max-10^{-80}
    arb_sub(PS_M_ratio_max,PS_M_ratio_max,t_cl,prec);
    printf("Find PS_M_ratio_max: ");arb_printn(PS_M_ratio_max, 50,0);printf("\n\n");
    arb_clear(t_cl);
    //exit(0);
    
    
    //考虑所有k模式的影响，利用δ谱的方式来求连续谱的PBHs
    //有两种方法：①利用δ谱参数计算，速度快，估算，太宽不准确；②利用连续谱参数计算，速度慢，准确
    //因与前面计算程序共用了数据，这里需传递设定几个参数
    //利用δ谱的x_m，求连续谱的特征模式 k_ch
    arb_set_str(Continuum_spectrum_x_m, "2.743707269992269382561122",prec); //对于不同的非高斯性，首先用δ谱的情况算出x_m
    arb_div(Continuum_spectrum_k_ch,Continuum_spectrum_x_m,R_MAX,prec); //连续谱的特征模式 k_ch
    
    //功率谱类型： lognormal_type/power_law_type/box_type/broken_power_law_type/link_cmb_type
    Continuum_spectrum_type=lognormal_type; //计算连续谱类型
    
    Continuum_spectrum_cal_simplify=false; //是否采用简化，true/false，这里简化现阶段仅针对log-normal谱
    
    
    //
    //全部设置完，后面调用各函数进行各种设算
    //
    
    arb_t Pk,t,w,k,eta; //自由使用变量
    arb_init(Pk);
    arb_init(t);
    arb_init(w);
    arb_init(k);
    arb_init(eta);
    
    //Ln_K_star=30.37829203018403957048
    //K_star=1.56E13
    arb_set_str(t,"0",prec);
    arb_set_str(w,"1E-12",prec);
    //arb_log(w,w,prec);
    //arb_set_str(Mu_2,"0.4",prec); //后面要输出ζ(r)、ζ_G(r)和C(r),这里不能赋值，用前面 Mu_2_th
    //arb_sub(w,Ln_K_star,w,prec);
    arb_add(t,t,Ln_K_star,prec);
    
    //power_spectrum(Pk,t,prec);
    //Help_sigma_n_square(Pk,4,prec);
    //Help_psi_1_n(Pk,t,3,prec);
    //Help_psi_1_n(Pk,t,0,prec);
    //Help_Laplacian_psi_1_n(Pk,t,0,prec);
    //zeta_Gauss_profile_n(Pk,w,0,prec);
    //zeta_profile_n(Pk,w,0,prec);
    //arb_mul(Pk,w,t,prec);
    //Find_r_max(t, prec);
    //C_m_average(Pk,t,prec);
    //arb_printn(Pk,60,0);printf("\n");
    //Find_Mu_2_th(Pk,prec);
    //C_r_profile_n(Pk,t,0,prec);
    //C_r_profile_n(Pk,w,1,prec);
    
    //arb_exp(t,t,3*prec);
    //arb_mul(w,w,t,3*prec);
    //arb_mul_si(w,Gamma_3,1,prec);
    //N_pk_f_xi(w,t,prec);
    //Help_psi_1_n(w,t,1,prec);
    
    //arb_sqrt(w,Gamma_3,prec);
    //Peak_number_density(Pk,t,w,prec);
    
    //arb_div(w,Sigma_1_square,Sigma_2_square,prec);
    //C_r_profile_n(Pk,t,1,prec);
    //Q_parameter(w,t,prec);
    //Delta_c_q_parameter(Pk,w,prec);
    //Horizon_reentry_mu_M(Pk,t, prec);
    //C_m_average(t,Pk,prec);
    //Horizon_reentry_M_ratio(Pk,t,prec);
    //PBH_number_density_M(Pk,t,prec);
    //arb_div(Pk,Pk,w,prec);
    //PBH_abundance_f_to_M(Pk,w,prec);
    
    
    
    //PS相关
    //PS_M_ratio_to_C_l(Pk,w,prec);
    //interior_probability_C_l(Pk,t,w,0,prec);
    //probability_gauss_2D(Pk,t,w,prec);
    //PS_abundance_beta_m(Pk,t,prec);
    //PS_abundance_beta_all(w,prec);
    //PS_abundance_f_m(Pk, w, prec);
    //PS_abundance_f_all(Pk,prec);
    //Probability_C(Pk,w,prec);
    //PS_abundance_beta_delta_k(Pk,w,prec);
    
    arb_printn(w,30,0);printf("\n");
    
    
    //考虑所有k模式，用δ谱计算连续谱
    //PS_abundance_beta_delta_k(Pk, t, prec); //计算某个k的β，临界坍缩的贡献都归到该k模式，传递值为ln(k)
    //PS_abundance_beta_delta_k_all(Pk, prec); //计算所有k的总β，传递值为ln(k)
    //PS_abundance_beta_delta_k_M(Pk,t,prec); //计算某个质量M(k)的β，考虑各个k的临界坍缩，传递值为ln(k)
    
    //诱导引力波
    arb_set_str(k,"1.56E12",prec);
    //arb_set_str(eta,"1",prec);
    arb_inv(eta,k,prec);
    //GW_power_spectra(Pk,eta,k,prec); //这里传入的k值未取对数
    GW_current_energy_density(Pk,k,prec);
    
    //arb_div(w,Pk,w,prec);
    //arb_printn(t,60,0);printf("\n");
    
    //arb_printn(t,60,0);printf("\n");
    //arb_printn(w,60,0);printf("\n");
    arb_printn(Pk,30,0);printf("\n");
    //sleep(600);
    exit(0);
    
    
    //输出计算结果到文件
    FILE * fpdate;
    fpdate = fopen(Out_date_file, "a"); //打开文件，a追加，w重新写入
    if( fpdate == NULL ) { //对文件打开操作进行判断
        printf("\n\nOpen Error: %s\t\n",Out_date_file);perror("file");printf("\n");
        exit(-1);
    }
    
    
    //先将前面的输入输出，再接β值
    fprintf(fpdate, "%s\t", argv[1]);
    arb_fprintn(fpdate,Mu_2_th,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,PS_C_th,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,PS_C_l_th,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,R_MAX,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,R_m_times_K,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,Continuum_spectrum_x_m,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,Continuum_spectrum_k_ch,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,Q_parameter_th,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,PS_Sigma_XX_save,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,PS_Sigma_XY_save,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,PS_Sigma_YY_save,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,w,20,ARB_STR_NO_RADIUS);fprintf(fpdate, "\t");
    arb_fprintn(fpdate,Pk,20,ARB_STR_NO_RADIUS);
    fprintf(fpdate, "\n");
    
    fclose(fpdate); //关闭文件
    
    
    //arb_set_str(PS_Sigma_YY,"0.02",prec); //算P(ζ)用
    
    
    
    //draw_pic(argv[1],prec); //输出点用于画图，可从命令行传递参数
    
    return 0;
    
}


