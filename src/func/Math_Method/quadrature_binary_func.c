#include "quadrature_binary_func.h"
#include "quadrature.h"
#include <stdlib.h>


//二元函数对y进行积分 自适应计算 迭代版本 iterate

//二元函数 Gauss–Kronrod积分法，自适应 对y积分用
static int binary_integration_gauss_kronrod_cal_y(arb_t res, const arb_t f_x,
                                  my_calc_func_binary func, void *param, const slong order,
                                  const arb_t a, const arb_t b, const arb_t error, arb_t geterro,
                                  slong prec) //计算积分
{
    
    arb_t s,t,x,aa,bb,sum_1,sum_2;
    
    arb_init(s);
    arb_init(t);
    arb_init(x);
    arb_init(aa);
    arb_init(bb);
    arb_init(sum_1);
    arb_init(sum_2);
    
    //积分区间[a,b]转到[-1,1]
    //I=(b-a)/2 * w_i*func[(b-a)/2*x_i+(b+a)/2]
    arb_sub(aa,b,a,prec);
    arb_div_si(aa,aa,2,prec);
    
    arb_add(bb,b,a,prec);
    arb_div_si(bb,bb,2,prec);
    
    extern arb_ptr INT_GUSS_KRONROD_COFFI;
    
    //65个点的版本
    for (int i=0; i < 65; i++)
    {
        arb_mul(x,aa,INT_GUSS_KRONROD_COFFI+i,prec);
        arb_add(x,x,bb,prec);
        
        //arb_printn(s, 50,0);printf("\n"); //打印变量
        
        func(s,f_x,x,param,order,prec); //函数值还会再用
        arb_mul(t,s,INT_GUSS_KRONROD_COFFI+65+i,prec);
        
        arb_add(sum_1,sum_1,t,prec);
        
        if( i%2==1 )
        {
            arb_mul(t,s,INT_GUSS_KRONROD_COFFI+129+(i+1)/2,prec);
            
            arb_add(sum_2,sum_2,t,prec);
        }
        
        
    }
    
    arb_mul(sum_1,sum_1,aa,prec);
    arb_mul(sum_2,sum_2,aa,prec);
    
    //误差判定
    arb_set(res,sum_1);
    arb_sub(s,sum_1,sum_2,prec);
    arb_abs(s,s);
    
    arb_set(geterro,s); //返回相应的误差大小
    
    
    if( arb_le(s,error) )
    {
        arb_clear(s);
        arb_clear(t);
        arb_clear(x);
        arb_clear(aa);
        arb_clear(bb);
        arb_clear(sum_1);
        arb_clear(sum_2);
        
        return 0;
    }else{
        
        arb_clear(s);
        arb_clear(t);
        arb_clear(x);
        arb_clear(aa);
        arb_clear(bb);
        arb_clear(sum_1);
        arb_clear(sum_2);
        
        return 1;
    }
    
    
}


//二元函数对y进行积分 ∫f(x,y)dy
static int binary_integration_gauss_kronrod_iterate_y(arb_t res, const arb_t f_x,
                                      my_calc_func_binary func, void *param, const slong order,
                                      const arb_t a, const arb_t b, const arb_t error,
                                      slong step_min , slong step_max,
                                      slong prec)
{
    arb_ptr as, bs, es,gs;
    arb_t s, u, w;
    
    slong depth,top,alloc,leaf_interval_count;
    int judge,stopping, status;
    
    
    depth=1;
    stopping = 0;
    status=0;
    alloc = 4;
    leaf_interval_count=0;
    
    as = _arb_vec_init(alloc); //初始化
    bs = _arb_vec_init(alloc);
    es = _arb_vec_init(alloc); //参考误差
    gs = _arb_vec_init(alloc); //计算得到的实际误差
    
    arb_set(as, a);//初始积分区间
    arb_set(bs, b);
    arb_set(es, error); //用来存放计算所需对比的误差
    arb_pos_inf(gs); //初始化计算误差
    
    arb_init(s);//积分求和用
    arb_init(u);
    arb_init(w);
    
    arb_pos_inf(w);//最初设其为正无穷大，用于最小迭代次数内设置计算误差
    arb_zero(u); //最初设为零，用于最小迭代次数内设置计算结果
    arb_zero(s);
    
    int time_min;
    time_min=0; //判断最小迭代次数
    
    while (depth >= 1)
    {
        
        //超过最大迭代次数，终止
        if (stopping == 0 && depth >= step_max )
        {
            flint_printf("\n达到最大迭代深度：  %wd\n\n", step_max);
            status++;
            stopping = 1;
            
            /*
             *           for(slong i=0;i<=depth;i++)
             *           {
             *               //arb_sub(u,bs+i,as+i,prec);
             *               arb_printn(gs+i, 15,0);printf("\n");
        }
        exit(0);
        */
        }
        
        
        top = depth - 1;
        
        if ( stopping) //达到最大迭代后，结束
        {
            //直接计算每个区间的值，加起来
            binary_integration_gauss_kronrod_cal_y(u,f_x,func,param,order,as+top,bs+top,es+top,w,prec);
            
            arb_add(s, s, u, prec);
            
            leaf_interval_count++;
            
            depth--;
            
            continue;
        }
        
        //有最小的最初次数要求
        if( time_min==0  && depth >= step_min   )
        {
            time_min=1;
        }
        
        if( time_min ) //有最小积分区间间隔要求
        {
            judge = binary_integration_gauss_kronrod_cal_y(u,f_x,func,param,order,as+top,bs+top,es+top,w,prec);
            
            if( judge==0)
            {
                //满足精度要求
                arb_add(s, s, u, prec);
                
                leaf_interval_count++;
                
                depth--;
                
                continue;
            }
        }
        
        //看数组存储是否够用，不够再增加分配
        if (depth >= alloc - 1)
        {
            slong k;
            as = flint_realloc(as, 2 * alloc * sizeof(arb_struct));
            bs = flint_realloc(bs, 2 * alloc * sizeof(arb_struct));
            es = flint_realloc(es, 2 * alloc * sizeof(arb_struct));
            gs = flint_realloc(gs, 2 * alloc * sizeof(arb_struct));
            for (k = alloc; k < 2 * alloc; k++)
            {
                arb_init(as + k); //分配空间后初始化
                arb_init(bs + k);
                arb_init(es + k);
                arb_init(gs + k);
            }
            alloc *= 2;
        }
        
        //二分区间
        /* Interval [depth] becomes [mid, b]. */
        arb_set(bs + depth, bs+top);
        arb_add(as + depth, as+top, bs+top, prec);
        arb_mul_2exp_si(as + depth, as + depth, -1);
        
        /* Interval [0] becomes [a, mid]. */
        arb_set(bs+top, as + depth);
        
        //区间误差更新
        arb_mul_2exp_si(es+top, es+top, -1); // es/2
        arb_set(es + depth , es+top); // es/2
        
        
        //计算得到的区间误差更新
        arb_mul_2exp_si(gs+depth, w, -1); //暴力处理，将原区间计算误差除以2        
        arb_set(gs+top, gs+depth);
        
        if(1)
        {
            //区间单隔，大的上升，小的沉底，按从小到大的顺序排列
            small2big_order(as, bs, es, gs, depth);
        }
        //printf("depth: %li\n",depth);
        
        depth++;
        //printf("deptht: %li\n",depth);
    }
    
    arb_set(res, s);
    
    //printf("leaf_interval_count: %li\n",leaf_interval_count);
    
    _arb_vec_clear(as, alloc);//释放内存
    _arb_vec_clear(bs, alloc);
    _arb_vec_clear(es, alloc);
    _arb_vec_clear(gs, alloc);
    arb_clear(s);
    arb_clear(u);
    arb_clear(w);
    
    return 0;
}



//二元函数对y进行积分 自适应计算 迭代版本 iterate

//二元函数 Gauss–Kronrod积分法，自适应 对y积分后再对x积分用
static int binary_integration_gauss_kronrod_cal_y_x(arb_t res,
                                           my_calc_func_binary func, void *param, const slong order,
                                           my_calc_func y_a_func, void *param_y_a,  const slong order_y_a,
                                           my_calc_func y_b_func, void *param_y_b,  const slong order_y_b,
                                           const arb_t y_error, slong y_step_min , slong y_step_max,
                                           const arb_t a, const arb_t b, const arb_t error, arb_t geterro,
                                           slong prec) //计算积分
{
    arb_t y_a,y_b; //对于每个x，都需要计算对y积分的上下限
    arb_init(y_a);
    arb_init(y_b);
    
    
    arb_t s,t,x,aa,bb,sum_1,sum_2;
    
    arb_init(s);
    arb_init(t);
    arb_init(x);
    arb_init(aa);
    arb_init(bb);
    arb_init(sum_1);
    arb_init(sum_2);
    
    //积分区间[a,b]转到[-1,1]
    //I=(b-a)/2 * w_i*func[(b-a)/2*x_i+(b+a)/2]
    arb_sub(aa,b,a,prec);
    arb_div_si(aa,aa,2,prec);
    
    arb_add(bb,b,a,prec);
    arb_div_si(bb,bb,2,prec);
    
    extern arb_ptr INT_GUSS_KRONROD_COFFI;
    
    
    //65个点的版本
    for (int i=0; i < 65; i++)
    {
        arb_mul(x,aa,INT_GUSS_KRONROD_COFFI+i,prec);
        arb_add(x,x,bb,prec);
        
        //arb_printn(s, 50,0);printf("\n"); //打印变量
        
        //此时，计算的函数值是对y积分后的结果
        //func(s,f_x,x,param,order,prec); //函数值还会再用
        
        //计算对y积分的上下限
        y_a_func(y_a,x,param_y_a,order_y_a,prec);
        y_b_func(y_b,x,param_y_b,order_y_b,prec);
        
        binary_integration_gauss_kronrod_iterate_y(s,x,func,param,order,
                                                   y_a,y_b,y_error,y_step_min,y_step_max,prec);
        
        arb_mul(t,s,INT_GUSS_KRONROD_COFFI+65+i,prec);
        
        arb_add(sum_1,sum_1,t,prec);
        
        if( i%2==1 )
        {
            arb_mul(t,s,INT_GUSS_KRONROD_COFFI+129+(i+1)/2,prec);
            
            arb_add(sum_2,sum_2,t,prec);
        }
        
        
    }
    
    arb_mul(sum_1,sum_1,aa,prec);
    arb_mul(sum_2,sum_2,aa,prec);
    
    //误差判定
    arb_set(res,sum_1);
    arb_sub(s,sum_1,sum_2,prec);
    arb_abs(s,s);
    
    arb_set(geterro,s); //返回相应的误差大小
    
    
    if( arb_le(s,error) )
    {
        arb_clear(y_a);
        arb_clear(y_b);
        arb_clear(s);
        arb_clear(t);
        arb_clear(x);
        arb_clear(aa);
        arb_clear(bb);
        arb_clear(sum_1);
        arb_clear(sum_2);
        
        return 0;
    }else{
        
        arb_clear(y_a);
        arb_clear(y_b);
        arb_clear(s);
        arb_clear(t);
        arb_clear(x);
        arb_clear(aa);
        arb_clear(bb);
        arb_clear(sum_1);
        arb_clear(sum_2);
        
        return 1;
    }
    
    
}


//∫[∫f(x,y)dy]dx
static int binary_integration_gauss_kronrod_iterate_y_x(arb_t res,
                                               my_calc_func_binary func, void *param,  const slong order,
                                               my_calc_func y_a_func, void *param_y_a,  const slong order_y_a,
                                               my_calc_func y_b_func, void *param_y_b,  const slong order_y_b,
                                               const arb_t y_error, slong y_step_min , slong y_step_max,
                                               const arb_t a, const arb_t b, const arb_t error,
                                               slong step_min , slong step_max,
                                               slong prec)
{
    arb_ptr as, bs, es,gs;
    arb_t s, u, w;
    
    slong depth,top,alloc,leaf_interval_count;
    int judge,stopping, status;
    
    
    depth=1;
    stopping = 0;
    status=0;
    alloc = 4;
    leaf_interval_count=0;
    
    as = _arb_vec_init(alloc); //初始化
    bs = _arb_vec_init(alloc);
    es = _arb_vec_init(alloc); //参考误差
    gs = _arb_vec_init(alloc); //计算得到的实际误差
    
    arb_set(as, a);//初始积分区间
    arb_set(bs, b);
    arb_set(es, error); //用来存放计算所需对比的误差
    arb_pos_inf(gs); //初始化计算误差
    
    arb_init(s);//积分求和用
    arb_init(u);
    arb_init(w);
    
    arb_pos_inf(w);//最初设其为正无穷大，用于最小迭代次数内设置计算误差
    arb_zero(u); //最初设为零，用于最小迭代次数内设置计算结果
    arb_zero(s);
    
    int time_min;
    time_min=0; //判断最小迭代次数
    
    while (depth >= 1)
    {
        
        //超过最大迭代次数，终止
        if (stopping == 0 && depth >= step_max )
        {
            flint_printf("\n达到最大迭代深度：  %wd\n\n", step_max);
            status++;
            stopping = 1;
            
            /*
             *           for(slong i=0;i<=depth;i++)
             *           {
             *               //arb_sub(u,bs+i,as+i,prec);
             *               arb_printn(gs+i, 15,0);printf("\n");
        }
        exit(0);
        */
        }
        
        
        top = depth - 1;
        
        if ( stopping) //达到最大迭代后，结束
        {
            //直接计算每个区间的值，加起来
            binary_integration_gauss_kronrod_cal_y_x(u,func,param,order,
                                                     y_a_func,param_y_a,order_y_a,
                                                     y_b_func,param_y_b,order_y_b,
                                                     y_error,y_step_min,y_step_max,
                                                     as+top,bs+top,es+top,w,prec);
            
            arb_add(s, s, u, prec);
            
            leaf_interval_count++;
            
            depth--;
            
            continue;
        }
        
        //有最小的最初次数要求
        if( time_min==0  && depth >= step_min   )
        {
            time_min=1;
        }
        
        if( time_min ) //有最小积分区间间隔要求
        {
            judge = binary_integration_gauss_kronrod_cal_y_x(u,func,param,order,
                                                             y_a_func,param_y_a,order_y_a,
                                                             y_b_func,param_y_b,order_y_b,
                                                             y_error,y_step_min,y_step_max,
                                                             as+top,bs+top,es+top,w,prec);
            
            if( judge==0)
            {
                //满足精度要求
                arb_add(s, s, u, prec);
                
                leaf_interval_count++;
                
                depth--;
                
                continue;
            }
        }
        
        //看数组存储是否够用，不够再增加分配
        if (depth >= alloc - 1)
        {
            slong k;
            as = flint_realloc(as, 2 * alloc * sizeof(arb_struct));
            bs = flint_realloc(bs, 2 * alloc * sizeof(arb_struct));
            es = flint_realloc(es, 2 * alloc * sizeof(arb_struct));
            gs = flint_realloc(gs, 2 * alloc * sizeof(arb_struct));
            for (k = alloc; k < 2 * alloc; k++)
            {
                arb_init(as + k); //分配空间后初始化
                arb_init(bs + k);
                arb_init(es + k);
                arb_init(gs + k);
            }
            alloc *= 2;
        }
        
        //二分区间
        /* Interval [depth] becomes [mid, b]. */
        arb_set(bs + depth, bs+top);
        arb_add(as + depth, as+top, bs+top, prec);
        arb_mul_2exp_si(as + depth, as + depth, -1);
        
        /* Interval [0] becomes [a, mid]. */
        arb_set(bs+top, as + depth);
        
        //区间误差更新
        arb_mul_2exp_si(es+top, es+top, -1); // es/2
        arb_set(es + depth , es+top); // es/2
        
        
        //计算得到的区间误差更新
        arb_mul_2exp_si(gs+depth, w, -1); //暴力处理，将原区间计算误差除以2        
        arb_set(gs+top, gs+depth);
        
        if(1)
        {
            //区间单隔，大的上升，小的沉底，按从小到大的顺序排列
            small2big_order(as, bs, es, gs, depth);
        }
        //printf("depth: %li\n",depth);
        
        depth++;
        //printf("deptht: %li\n",depth);
    }
    
    arb_set(res, s);
    
    //printf("leaf_interval_count: %li\n",leaf_interval_count);
    
    _arb_vec_clear(as, alloc);//释放内存
    _arb_vec_clear(bs, alloc);
    _arb_vec_clear(es, alloc);
    _arb_vec_clear(gs, alloc);
    arb_clear(s);
    arb_clear(u);
    arb_clear(w);
    
    return 0;
}

//二元函数对y进行积分后再对x积分 ∫[∫f(x,y)dy]dx
static void integration_binary_y_x(arb_t res, my_calc_func_binary func, void* param, const slong order,
                            const arb_t x_a, const arb_t x_b, const arb_t x_error, 
                            slong x_step_min , slong x_step_max,
                            my_calc_func y_a_func, void *param_y_a,  const slong order_y_a,
                            my_calc_func y_b_func, void *param_y_b,  const slong order_y_b,
                            const arb_t y_error, slong y_step_min , slong y_step_max,
                            slong prec)
{
    arb_t s;
    arb_init(s);
    
    binary_integration_gauss_kronrod_iterate_y_x(s,func,param,order,
                                                 y_a_func,param_y_a,order_y_a,
                                                 y_b_func,param_y_b,order_y_b,
                                                 y_error,y_step_min,y_step_max,
                                                 x_a,x_b,x_error,x_step_min,x_step_max,
                                                 prec);
    arb_set(res,s);
    arb_clear(s);
}


//二元积分，积分区域由函数表示
//非矩形区域 ∫_{x_a}^{x_b} dx ∫_{y_a(x)}^{y_b(x)} dy f(x,y)
int integration_binary_func(arb_t res, my_calc_func_binary func, void *param, const slong order,
                       const arb_t x_a, const arb_t x_b, const arb_t x_error, 
                       slong x_step_min , slong x_step_max,
                       my_calc_func y_a_func, void *param_y_a,  const slong order_y_a,
                       my_calc_func y_b_func, void *param_y_b,  const slong order_y_b,
                       const arb_t y_error, slong y_step_min, slong y_step_max,
                       slong prec)
{
    arb_t s;
    arb_init(s);
    
    get_gauss_kronrod_node_weight(65,prec); //获取 gauss_kronrod 的节点位置和权重
    
    //对于二元函数 f(x,y)
    //先对y积分 ∫f(x,y)dy
    //再对x进行积分 ∫[∫f(x,y)dy]dx
    
    integration_binary_y_x(s,func,param,order,
                           x_a,x_b,x_error,x_step_min,x_step_max,
                           y_a_func,param_y_a,order_y_a,
                           y_b_func,param_y_b,order_y_b,
                           y_error,y_step_min,y_step_max,
                           prec);
    
    arb_set(res,s);
    arb_clear(s);
    return 0;
}
