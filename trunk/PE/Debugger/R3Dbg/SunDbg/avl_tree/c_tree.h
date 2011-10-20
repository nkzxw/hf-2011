// c_tree.h: interface for the c_tree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_C_TREE_H__9D944A15_A3A3_447E_BA8F_1FBCDEAF0896__INCLUDED_)
#define AFX_C_TREE_H__9D944A15_A3A3_447E_BA8F_1FBCDEAF0896__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// #include <stdio.h>
// #include <iostream.h>

template <typename T>
class node
{
public:
    T data;
    node* p_parent;
    node* p_left;
    node* p_right;
    int bf;//ƽ������ balance factor
    int depth;
public:
    node()
    {}
    node(T newdata)//������ڵ�
    {
        data = newdata;
        depth = 1;
        p_left = p_right = NULL;
        p_parent = NULL;
        bf = 0;
    }
    node(node<T>* p_node, T newdata)//����Ǹ��ڵ�
    {
        data = newdata;
        depth = p_node->depth + 1;
        p_parent = p_node;
        p_left = p_right = NULL;
        bf = 0;
    }
    void print_1()//�ȸ����
    {
        if(this)
        {
//             cout<<"address : "<<this<<" data :"<<data<< " "<<this->bf<<" ";
//             if(p_parent) {cout<<"\tparent:"<<p_parent->data;}
//             if(p_left)   {cout<<"\tleft:  "<<p_left->data;}
//             if(p_right)  {cout<<"\tright: "<<p_right->data;}
//             cout<<endl;
            cout<<data<<" "<<this->bf<<endl;
        }
    }
    void print_2()//�и����
    {
        if(this)
        {
            cout<<data<<" ";
//             cout<<"myselft:"<<hex<<data;
//             if(p_left)   {cout<<"\tleft:  "<<p_left->data;}
//             if(p_parent) {cout<<"\tparent:"<<p_parent->data;}
//             if(p_right)  {cout<<"\tright: "<<p_right->data;}
//             cout<<endl;
        }
    }
    void print_3()//������
    {
        if(this)
        {
            cout<<"myselft:"<<data;
            if(p_left)   {cout<<"\tleft:  "<<p_left->data;}
            if(p_right)  {cout<<"\tright: "<<p_right->data;}
            if(p_parent) {cout<<"\tparent:"<<p_parent->data;}
            cout<<endl;
        }
    }
};

template <typename T>
class c_tree
{
private:
    node<T>* p_root;
    int depth;
    int i_count;
public:
	c_tree()
    {
        p_root = NULL;
        depth = 0;
        i_count = 0;
    }
	~c_tree()//�ͷ�������ռ�ڴ�ռ�
    {
//         cout<<"��ʼ�ȸ��ͷ��ڴ棡"<<endl;
        release_all_DLR(p_root);
    }
public:
    node<T>* get_root()
    {
        return p_root;
    }

    int get_count()
    {
        return i_count;
    }

    node<T>* insert(node<T>* p_node, T newdata)
    {
        node<T>* p_new = NULL;
        if(NULL == p_node)
        {
            p_new = new node<T>(newdata);
            p_root = p_new;
            depth = 1;
        }
        else
        {
            p_new = new node<T>(p_node, newdata);
            if (p_node->p_left == NULL)
            {
                p_node->p_left = p_new;
            }
            else if(p_node->p_right == NULL)
            {
                p_node->p_right = p_new;
            }
            else
            {
                if (p_new)
                {
                    delete p_new;
                }
                p_new = NULL;
                return NULL;
            }
            if(depth < p_new->depth)
                depth = p_new->depth;
        }
        i_count++;
        return p_new;
    }

    node<T>* sort_insert(T newdata)
    {
        node<T>* p_new = NULL;
        if(NULL == this->p_root)
        {
            p_new = new node<T>(newdata);
            p_root = p_new;
            depth = 1;
        }
        else
        {
            int i_flag_LR = 0;//�ж��ǲ�����߻����ұߵ�һ�����ֵ
            node<T>* p_find = find_place(p_root, newdata, i_flag_LR);
            if (p_find == NULL)
            {
                return NULL;
            }
            p_new = new node<T>(newdata);
            if (i_flag_LR == 0)
            {
                p_find->p_left = p_new;
            }
            else
            {
                p_find->p_right = p_new;
            }
            p_new->p_parent = p_find;         
        }
        i_count++;
        return p_new;
    }

    node<T>* find_place(node<T>* p_node, T newdata, int &i_flag_LR)
    {
        node<T>* p_find = p_node;
        if (p_node)
        {
            if (p_node->data > newdata)
            {
                if (p_node->p_left)
                {
                    p_find = find_place(p_node->p_left, newdata, i_flag_LR);
                }
                else
                {
                    i_flag_LR = 0;
                }
            }
            else
            {
                if (p_node->p_right)
                {
                    p_find = find_place(p_node->p_right, newdata, i_flag_LR);
                }
                else
                {
                    i_flag_LR = 1;
                }
            }
        }
        return p_find;
    }

    //balance
    node<T>* balance_sort_insert(T newdata)
    {
        node<T>* p_new = NULL;
        node<T>* rotate_node = NULL;//��ת��

        if(NULL == this->p_root)
        {
            p_new = new node<T>(newdata);
            p_root = p_new;
            depth = 1;
            p_new->bf = 0;
        }
        else
        {
            int i_flag_LR = 0;//�ж��ǲ�����߻����ұߵ�һ�����ֵ
            node<T>* p_find = find_place(p_root, newdata, i_flag_LR);
            if (p_find == NULL)
            {
                return NULL;
            }
            p_new = new node<T>(newdata);
            p_new->p_parent = p_find;
            p_new->bf = 0;

            if (i_flag_LR == 0)//��Ϊ���Ӳ���
            {
                p_find->p_left = p_new;
            }
            else//��Ϊ�Һ��Ӳ���
            {
                p_find->p_right = p_new;
            }

            //������㷽��������޸�ƽ�����ӣ�����ƽ�����Ӿ���ֵ����1ʱ��������ת
            //������ƽ�����Ӳ��ڱ仯ʱ��ֹͣ�����㷽��ı���
            node<T>* tmp = p_new;
            while(tmp->p_parent)
            {
                if(tmp->p_parent->bf == -1)         //tmp->p_parent->bf == -1
                {
                    if (tmp == tmp->p_parent->p_left)
                    {
                        tmp->p_parent->bf = 0;
                        break;
                    }
                    else
                    {
                        tmp->p_parent->bf = -2;
                        rotate_node = tmp->p_parent;
                        break;
                    }
                }
                else if(tmp->p_parent->bf == 0)     //tmp->p_parent->bf == 0
                {
                    if (tmp == tmp->p_parent->p_left)
                    {
                        tmp->p_parent->bf = 1;
                        tmp = tmp->p_parent;
                    }
                    else
                    {
                        tmp->p_parent->bf = -1;
                        tmp = tmp->p_parent;
                    }
                }
                else                                //tmp->p_parent->bf == 1
                {
                    if (tmp == tmp->p_parent->p_left)
                    {
                        tmp->p_parent->bf = 2;
                        rotate_node = tmp->p_parent;
                        break;                    	
                    }
                    else
                    {
                        tmp->p_parent->bf = 0;
                        break;
                    }
                }
            }

            if (rotate_node)//�ҵ���ת�㣬��Ҫ��ת
            {
            	if (rotate_node->bf == 2)//����������������2��
            	{
            		if (rotate_node->p_left->bf == 1)//LL��
            		{
            			//cout<<"LL�͵���"<<endl;
                        rotate_LL(rotate_node);
            		}
            		else//LR��  rotate_node->p_left->bf == -1
            		{
            			//cout<<"LR�͵���"<<endl;
                        rotate_LR(rotate_node);
            		}
            	} 
            	else//rotate_node->bf == -2 //����������������2��
            	{
            		if (rotate_node->p_right->bf == 1)//RL��
            		{
            			//cout<<"RL�͵���"<<endl;
                        rotate_RL(rotate_node);
            		}
            		else//RR��  rotate_node->p_right->bf == -1
            		{
            			//cout<<"RR�͵���"<<endl;
                        rotate_RR(rotate_node);
            		}
            	}
            }
        }
        i_count++;
        return p_new;
    }

    //LR�͵���(���������ұ��أ���������������أ�)��Ҳ���Ǵ���������˵����أ��Ӿֲ���˵�ұ���
    //�ȴ���ֲ����ڴ������壬������������������
    void rotate_LR(node<T>* balance_node)
    {
        rotate_RR(balance_node->p_left);
        rotate_LL(balance_node);
    }

    void rotate_RL(node<T>* balance_node)
    {
        rotate_LL(balance_node->p_right);
        rotate_RR(balance_node);
    }
    
//          8        ����0��8��ƽ�����Ӵ�1��Ϊ2
//         / \
//        6   10     ����0��6��ƽ�����Ӵ�0��Ϊ1
//      /  \   \
//     2    7   11   ����0��2��ƽ�����Ӵ�0��Ϊ1
//    / \  /  \      
//   1  4 6.5 7.5    ����0��1��ƽ�����Ӵ�0��Ϊ1
//  /
// (0)

//           6             ������6��ƽ�����Ӵ�1��Ϊ0
//         /   \
//       2      8          ������8��ƽ�����Ӵ�2��Ϊ0
//      / \    /  \ 
//     1   4  7   10
//    /      /  \    \
//   0      6.5 7.5   11 


    //LL�͵���������أ�������ת��
    void rotate_LL(node<T>* balance_node)
    {
        if (balance_node == p_root)
        {
        	p_root = balance_node->p_left;
        } 
        //  balance_node = 8
        node<T>* balance_node_parent = balance_node->p_parent;//  null
        node<T>* balance_node_left = balance_node->p_left;    //  6,��ֵһ������
        node<T>* balance_node_left_right = balance_node->p_left->p_right; //  7��ֵ�п���Ϊ��
        //  6����8����
        balance_node_left->p_right = balance_node;
        balance_node->p_parent = balance_node_left;
        balance_node_left->p_parent = balance_node_parent;
        if(balance_node_parent)
        {
            (balance_node_parent->p_left == balance_node)?
            (balance_node_parent->p_left = balance_node_left):
            (balance_node_parent->p_right = balance_node_left);
        }
        //  ����7�Ĺ�ϵ
        balance_node->p_left = balance_node_left_right;
        if(balance_node_left_right)
        {
            balance_node_left_right->p_parent = balance_node;
        }
        //�޸���ת�㣨8������ת�����ӽ�㣨6����ƽ������
        if (balance_node->bf == 2)
        {
            if (balance_node_left->bf == 1)
            {
                balance_node->bf = 0;
                balance_node_left->bf = 0;
            } 
            else if( balance_node_left->bf == 2)
            {
                balance_node->bf = -1;
                balance_node_left->bf = 0;            	
            }
            else if( balance_node_left->bf == 0)//ɾ����ʱ�����������
            {
                balance_node->bf = 1;
                balance_node_left->bf = -1;            	
            }
        } 
        else if(balance_node->bf == 1) //RL��ת�е�����ת��˫��ת�еĵ�һ�Σ�
        {
        	if (balance_node_left->bf == 1)
        	{
                balance_node_left->bf = -1;
                balance_node->bf = -1;
        	} 
        	else if ( balance_node_left->bf == -1)
        	{
                balance_node_left->bf = -2;
                balance_node->bf = 0;
        	}
            else if ( balance_node_left->bf == 0)
            {
                balance_node_left->bf = -1;
                balance_node->bf = 0;
            }
        }
    }

    //RR�͵������ұ��أ�������ת��
    void rotate_RR(node<T>* balance_node)
    {
        if (balance_node == p_root)
        {
        	p_root = balance_node->p_right;
        } 
        node<T>* balance_node_parent = balance_node->p_parent;
        node<T>* balance_node_right = balance_node->p_right;    //  ��ֵһ������
        node<T>* balance_node_right_left = balance_node->p_right->p_left; //  ��ֵ�п���Ϊ��
        balance_node_right->p_left = balance_node;
        balance_node->p_parent = balance_node_right;
        balance_node_right->p_parent = balance_node_parent;
        if(balance_node_parent)
        {
            (balance_node_parent->p_left == balance_node)?
            (balance_node_parent->p_left = balance_node_right):
            (balance_node_parent->p_right = balance_node_right);
        }
        //  ������ת�����ӽڵ�����ӽ��
        balance_node->p_right = balance_node_right_left;
        if(balance_node_right_left)
        {
            balance_node_right_left->p_parent = balance_node;
        }
        //�޸� ��ת�� �� ��ת�����ӽ�� ��ƽ������
        if (balance_node->bf == -2)  
        {
            if (balance_node_right->bf == -1)    //��ͨ������ת
            {
                balance_node->bf = 0;
                balance_node_right->bf = 0;
            } 
            else if(balance_node_right->bf == -2) //˫��ת�е�����
            {
                balance_node->bf = 1;
                balance_node_right->bf = 0;            	
            }
            else if(balance_node_right->bf == 0)//ɾ����ʱ�����������
            {
                balance_node->bf = -1;
                balance_node_right->bf = 1;            	
            }

        } 
        else if(balance_node->bf == -1) //LR��ת�е�����ת 
        {
        	if (balance_node_right->bf == 1)
        	{
                balance_node_right->bf = 2;
                balance_node->bf = 0;
        	} 
        	else if(balance_node_right->bf == -1)
        	{
                balance_node_right->bf = 1;
                balance_node->bf = 1;
        	}
            else if(balance_node_right->bf == 0)
            {
                balance_node_right->bf = 1;
                balance_node->bf = 0;
            }
        }
    }

    node<T>* find_data(T data)
    {
        return find(p_root, data);
    }

    node<T>* find(node<T>* p_node, T data)
    {
        if (p_node)
        {
            if(p_node->data == data)
            {
                return p_node;
            }
            else if(data < p_node->data)
            {
                return find(p_node->p_left, data);
            }
            else
            {
                return find(p_node->p_right, data);
            }            
        }
        else
        {
            return NULL;
        }        
    }

    bool balance_delete_node(node<T>* p_node)
    {
        if (p_node)
        {
            node<T>* p_change_node = NULL;
            if (p_node->p_left == NULL && p_node->p_right == NULL)//��������Ϊ��
            {
                if (p_node == p_root)
                {
                    p_root = NULL;
                    i_count--;
                    return true;
                }
                if (p_node->p_parent)
                {
                    if(p_node->p_parent->p_left == p_node)
                    {
                        p_node->p_parent->p_left = NULL;
                        p_node->p_parent->bf = p_node->p_parent->bf - 1;
                    }
                    else
                    {
                        p_node->p_parent->p_right = NULL;
                        p_node->p_parent->bf = p_node->p_parent->bf + 1;
                    }
                }
                p_change_node = p_node->p_parent;
            }
        	else if (p_node->p_left == NULL && p_node->p_right != NULL)//������Ϊ��
        	{
                p_node->p_right->p_parent = p_node->p_parent;

                if (p_node == p_root)
                {
                    p_root = p_node->p_right;
                }
                //����p_node�ĸ�����ƽ�����ӣ���������ݣ����Ƿ���Ҫ��ת
                //p_node��λ�ñ�p_node->p_right���档
                if (p_node->p_parent)
                {
                    if(p_node->p_parent->p_left == p_node)
                    {
                        p_node->p_parent->p_left = p_node->p_right;
                        p_node->p_parent->bf = p_node->p_parent->bf - 1;
                    }
                    else
                    {
                        p_node->p_parent->p_right = p_node->p_right;
                        p_node->p_parent->bf = p_node->p_parent->bf + 1;
                    }
                } 
                p_change_node = p_node->p_parent;
        	} 
        	else if(p_node->p_right == NULL && p_node->p_left != NULL)//������Ϊ��
        	{
                p_node->p_left->p_parent = p_node->p_parent;

                if (p_node == p_root)
                {
                    p_root = p_node->p_left;
                }
                //����p_node�ĸ�����ƽ�����ӣ���������ݣ����Ƿ���Ҫ��ת
                //p_node��λ�ñ�p_node->p_left���档
                if (p_node->p_parent)
                {
                    if(p_node->p_parent->p_left == p_node)
                    {
                        p_node->p_parent->p_left = p_node->p_left;
                        p_node->p_parent->bf = p_node->p_parent->bf - 1;
                    }
                    else
                    {
                        p_node->p_parent->p_right = p_node->p_left;
                        p_node->p_parent->bf = p_node->p_parent->bf + 1;
                    }
                } 
                p_change_node = p_node->p_parent;
        	}
            else//������������Ϊ��
            {
                //��p_node��ֱ��ǰ׺����p_node
                node<T>* p_tmp = p_node->p_left;//p_node->p_left�϶�����
                while(p_tmp->p_right)
                {
                    p_tmp = p_tmp->p_right;
                }

                if (p_node == p_root)
                {
                    p_root = p_tmp;
                }

                //������p_tmp == p_node->p_left, p_tmp��������Ϊ��
                if (p_tmp == p_node->p_left)
                {
                    //p_tmp����p_node
                    p_tmp->p_right = p_node->p_right;
                    p_node->p_right->p_parent = p_tmp;
                	p_tmp->p_parent = p_node->p_parent;
                    if (p_node->p_parent)
                    {
                        if (p_node->p_parent->p_left == p_node)
                        {
                            p_node->p_parent->p_left = p_tmp;
                        }
                        else
                        {
                            p_node->p_parent->p_right = p_tmp;
                        }
                    }
                    p_tmp->bf = p_node->bf - 1;
                    p_change_node = p_tmp;
                } 
                else//һ�����
                {
                    //����tmp�ĸ������ҽ��ָ��tmp������
                    p_tmp->p_parent->p_right = p_tmp->p_left;
                    if (p_tmp->p_left)//����tmp������ĸ����ָ��tmp�ĸ����
                    {
                        p_tmp->p_left->p_parent = p_tmp->p_parent;
                    }
                    p_tmp->p_parent->bf = p_tmp->p_parent->bf + 1;
                    p_change_node = p_tmp->p_parent;

                    p_tmp->p_parent = p_node->p_parent;
                    p_tmp->bf = p_node->bf;//p_tmpռ��p_node��λ�ú󣬼̳�p_node��BF

                    if (p_node->p_parent)
                    {
                        if (p_node->p_parent->p_left == p_node)
                        {
                            p_node->p_parent->p_left = p_tmp;
                            //p_node->p_parent->bf = p_node->p_parent->bf - 1;
                        }
                        else
                        {
                            p_node->p_parent->p_right = p_tmp;
                            //p_node->p_parent->bf = p_node->p_parent->bf + 1;
                        }
                    }

                    p_tmp->p_left = p_node->p_left;
                    p_tmp->p_right = p_node->p_right;

                    p_node->p_left->p_parent = p_tmp;
                    p_node->p_right->p_parent = p_tmp;
                }
            }
            
            if (p_change_node)//���ݽ���ƽ���Լ��
            {
                balance_adjust(p_change_node);
            }

            delete p_node;
            p_node = NULL;
            i_count--;
            return true;
        } 
        else
        {
        	return false;
        }
    }

    void balance_adjust(node<T>* p_node)
    {
        if (p_node)
        {
            bool p_node_is_left_of_parent = true;
            node<T>* p_node_parent = p_node->p_parent;
            node<T>* p_node_copy = p_node;
            //�������䣬֮����Ҫ����p_node���丸��㣬����Ϊ����ת���̵��л��ƻ����ǵ�������ϵ
            //ͬ��������ı���p_node_is_left_of_parent��¼p_node�Ƿ��Ǹ���������
            //������ǰ�жϣ�������ת֮�����ǿ��ܲ��Ǹ��ӽ��Ĺ�ϵ
            if (p_node_parent)
            {
                p_node_is_left_of_parent = (p_node == p_node_parent->p_left);
            }
            if (p_node->bf == 1 || p_node->bf == -1)//�����ޱ仯
            {
                return;        	
            } 
            else if(p_node->bf == 0)//���߽��ͣ���Ӱ�츸����BF
            {
                //����������BF
                if (p_node_parent)
                {
                    (p_node_is_left_of_parent) ?
                        (p_node_parent->bf = p_node_parent->bf - 1) :
                        (p_node_parent->bf = p_node_parent->bf + 1);
                }
                balance_adjust(p_node_parent);
            }
            else if(p_node->bf == 2)//��Ҫ��ת
            {
                if (p_node->p_left->bf == 1)//LL��ת  ���߽��ͣ����������
                {
                	rotate_LL(p_node);//ע�⣬��ת֮��ı���p_node�ĸ��ӽ��
                    //����������BF
                    if (p_node_parent)
                    {
                        (p_node_is_left_of_parent) ?
                            (p_node_parent->bf = p_node_parent->bf - 1) :
                            (p_node_parent->bf = p_node_parent->bf + 1);
                    }
                    balance_adjust(p_node_parent);
                } 
                else if(p_node->p_left->bf == -1)//LR˫��  ���߽��ͣ����������
                {
                	rotate_LR(p_node);
                    //����������BF
                    if (p_node_parent)
                    {
                        (p_node_is_left_of_parent) ?
                            (p_node_parent->bf = p_node_parent->bf - 1) :
                            (p_node_parent->bf = p_node_parent->bf + 1);
                    }
                    balance_adjust(p_node_parent);
                }
                else if(p_node->p_left->bf == 0)//LL��ת  ���߲��䣬����ֹͣ
                {
                	rotate_LL(p_node);
                    return;
                }                
            }
            else if(p_node->bf == -2)//��Ҫ��ת
            {
                if (p_node->p_right->bf == -1)//RR��ת  ���߽��ͣ����������
                {
                	rotate_RR(p_node);
                    //����������BF
                    if (p_node_parent)
                    {
                        (p_node_is_left_of_parent) ?
                            (p_node_parent->bf = p_node_parent->bf - 1) :
                            (p_node_parent->bf = p_node_parent->bf + 1);
                    }
                    balance_adjust(p_node_parent);
                } 
                else if(p_node->p_right->bf == 1)//RL˫��  ���߽��ͣ����������
                {
                	rotate_RL(p_node);
                    //����������BF
                    if (p_node_parent)
                    {
                        (p_node_is_left_of_parent) ?
                            (p_node_parent->bf = p_node_parent->bf - 1) :
                            (p_node_parent->bf = p_node_parent->bf + 1);
                    }
                    balance_adjust(p_node_parent);
                }
                else if(p_node->p_right->bf == 0)//RR��ת  ���߲��䣬����ֹͣ
                {
                	rotate_RR(p_node);
                    return;
                }
            }
        }
        else
        {
            return;
        }
    }


    void print_DLR(node<T>* p_node)//�ȸ����
    {
        if (!p_node)
        {
            return;
        }
        p_node->print_1();
        if(p_node->p_left)
        {
            print_DLR(p_node->p_left);
        }
        if(p_node->p_right)
        {
            print_DLR(p_node->p_right);
        }
    }

    void print_LDR(node<T>* p_node)//�и����
    {
        if (!p_node)
        {
            return;
        }
        if(p_node->p_left)
        {
            print_LDR(p_node->p_left);
        }
        p_node->print_1();
        if(p_node->p_right)
        {
            print_LDR(p_node->p_right);
        }
    }

    void write_array(int* p, int* pbf, node<T>* p_node, int i_start, int i_end)//�и����������
    {
        int i_center = (i_start + i_end) / 2;
        if (p_node)
        {
            p[i_center-1] = p_node->data;
            pbf[i_center-1] = p_node->bf;
            write_array(p, pbf, p_node->p_left, i_start, i_center-1);
            write_array(p, pbf, p_node->p_right, i_center+1, i_end);
        }
    }

   void print_LRD(node<T>* p_node)//������
    {
        if (!p_node)
        {
            return;
        }
        if(p_node->p_left)
        {
            print_LRD(p_node->p_left);
        }
        if(p_node->p_right)
        {
            print_LRD(p_node->p_right);
        }
        p_node->print_3();
    }

    void release_all(node<T>* p_node)
    {
        if (p_node == NULL)
        {
            return;
        }

        if (p_node->p_left)
        {
            release_all(p_node->p_left);
        }
        if (p_node->p_right)
        {
            release_all(p_node->p_right);
        }
        if (p_node)
        {
            //cout<<"release "<<p_node->data<<endl;
            delete p_node;
            p_node = NULL;
        }        
    }

    bool delete_node(node<T>* p_node)//û�д���depth
    {
        if (!p_node)
        {
            return false;
        }
        if (!p_node->p_left)
        {
            if(p_node->p_right)
                p_node->p_right->p_parent = p_node->p_parent;
            if (p_node->p_parent)
            {
                (p_node->p_parent->p_right == p_node) ? 
                    (p_node->p_parent->p_right = p_node->p_right) :
                    (p_node->p_parent->p_left = p_node->p_right);
            }
            if (p_node == this->p_root)
            {
                p_root = p_node->p_right;
            }
        } 
        else if(!p_node->p_right)
        {
            if(p_node->p_right)
                p_node->p_right->p_parent = p_node->p_parent;
            if(p_node->p_parent)
            {
            (p_node->p_parent->p_right == p_node) ? 
                (p_node->p_parent->p_right = p_node->p_right) :
                (p_node->p_parent->p_left = p_node->p_right);
            }
            if (p_node == this->p_root)
            {
                p_root = p_node->p_left;
            }
        }
        else
        {
            node<T>* tmp = p_node->p_left;
            while(tmp->p_right)//tmpΪp_node���������������½�
            {
                tmp = tmp->p_right;
            }
            if (tmp == p_node->p_left)//���p_node���������������½Ǿ�������������㱾��
            {
                tmp->p_parent = p_node->p_parent;
                if(p_node->p_parent)
                {
                    (p_node->p_parent->p_right == p_node) ? 
                        (p_node->p_parent->p_right = tmp) :
                        (p_node->p_parent->p_left = tmp);
                }
                tmp->p_right = p_node->p_right;
                p_node->p_right->p_parent = tmp;
            }
            else
            {
                tmp->p_parent->p_right = tmp->p_left;//����tmp�ĸ������ҽ��ָ��tmp������
                if (tmp->p_left)//����tmp������ĸ����ָ��tmp�ĸ����
                {
                    tmp->p_left->p_parent = tmp->p_parent;
                }

                tmp->p_parent = p_node->p_parent;
                if(p_node->p_parent)
                {
                    (p_node->p_parent->p_right == p_node) ? 
                        (p_node->p_parent->p_right = tmp) :
                        (p_node->p_parent->p_left = tmp);
                }
                tmp->p_left = p_node->p_left;
                tmp->p_right = p_node->p_right;

                p_node->p_left->p_parent = tmp;
                p_node->p_right->p_parent = tmp;
            }
            if (p_node == this->p_root)
            {
                p_root = tmp;
            }
        }
        delete p_node;
        p_node = NULL;
        this->i_count--;
        return true;
    }

    void release_all_DLR(node<T>* p_node)
    {
        if (!p_node)
        {
            return;
        }
        node<T>* p_tmp_left = p_node->p_left;
        node<T>* p_tmp_right = p_node->p_right;
        if (p_node)
        {
//             cout<<"release "<<p_node->data<<endl;
            delete p_node;
            p_node = NULL;
        }
        if (p_tmp_left)
        {
            release_all_DLR(p_tmp_left);
        }
        if (p_tmp_right)
        {
            release_all_DLR(p_tmp_right);
        }
    }

};

#endif // !defined(AFX_C_TREE_H__9D944A15_A3A3_447E_BA8F_1FBCDEAF0896__INCLUDED_)
