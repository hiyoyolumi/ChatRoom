#include "func.h"

extern pthread_mutex_t mutex;

extern int num_birth;
extern int clients[1000];

void *serv_new_client(void *arg)
{
    pthread_detach(pthread_self());

    pthread_t thid;
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    int rows;
    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag;
    char username[20];
    char password[20];
    char nickname[20];
    char mibao[20];

    char buf[BUFSIZ];
    char query_str[BUFSIZ];
    char duff[BUFSIZ];

    while(1)
    {   
        strcpy(duff, "[1] 登陆\n[2] 注册\n[3] 找回密码\n[q] 关闭应用\n");
        Write(cm.cfd, duff);
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "1") == 0)         //登陆
        {
            strcpy(duff, "---请输入账号(q to quit):");
            Write(cm.cfd, duff);
            while(1)        //循环输入得到一个已被注册的用户名
            {
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                if(flag == 1)
                {
                    strcpy(duff, "---不存在此用户名，请重新输入(q to quit):");
                    Write(cm.cfd, duff);
                    continue;
                }
                strcpy(username, buf);
                
                strcpy(duff, "---请输入密码(q to quit):");
                Write(cm.cfd, duff);
                while(1)        //循环输入得到密码，如果输入正确的话进入用户界面
                {
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    flag = mysql_repeat(&cm.mysql, "UserData", buf, 2);
                    if(flag == 1)
                    {
                        strcpy(duff, "---密码输入错误，请重新输入(q to quit):");
                        Write(cm.cfd, duff);
                        continue;
                    }
                    //执行到这里说明账号密码输入正确
                    //把UserData中的status改为1
                    //然后来进入用户界面
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update UserData set status = 1 where username = \"%s\"", username);
                    rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                    if(rows != 0)
                    {
                        printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                        my_err("mysql_real_query error", __LINE__);
                    }
                    strcpy(cm.username, username);
                    if(pthread_create(&thid, NULL, func_yonghu, (void *)&cm) == -1)
                    {
                        my_err("pthread_create error", __LINE__);
                    }
                    pthread_join(thid, NULL);

                    break;
                }
                break;
            }
        }
        else if(strcmp(buf, "2") == 0)    //注册
        {
            strcpy(duff, "---请输入账号(q to quit):");
            Write(cm.cfd, duff);
            while(1)        //循环输入一个未被注册的用户名
            {
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                if(flag == 0)
                {
                    strcpy(duff, "---该用户名已被注册，请重新输入(q to quit):");
                    Write(cm.cfd, duff);
                    continue;
                }
                strcpy(username, buf);
                // printf("username = %s\n", username);
                strcpy(duff, "---请输入密码(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                strcpy(password, buf);
                // printf("password = %s\n", password);
                strcpy(duff, "---请输入昵称(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                strcpy(nickname, buf);
                // printf("nickname = %s\n", nickname);
                strcpy(duff, "---请输入密保(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                strcpy(mibao, buf);
                // printf("mibao = %s\n", mibao);

                //所有信息输入完毕，将得到的信息更新进UserData中
                memset(query_str, 0, sizeof(query_str));
                sprintf(query_str, "insert into UserData values\
                (\"%s\", \"%s\", \"%s\", \"%s\", \"%d\", \"0\", \"%d\", \"0\")", \
                username, password, nickname, mibao, num_birth, cm.cfd);
                rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                //更新num_birth的值
                pthread_mutex_lock(&mutex);
                    num_birth++;
                pthread_mutex_unlock(&mutex);
                
                //为这个新用户建一个好友列表（表）
                memset(query_str, 0, sizeof(query_str));
                sprintf(query_str, "create table %s(username varchar(20), num double)", username);
                rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                strcpy(duff, "---注册成功\n");
                Write(cm.cfd, duff);
                break;
            }
        }
        else if(strcmp(buf, "3") == 0)    //找回密码
        {
            while(1)        //循环得到UserData中存在的username
            {
                strcpy(duff, "请输入要找回的账号(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                if(flag == 1)
                {   
                    strcpy(duff, "---没有你要找回的账号");
                    Write(cm.cfd, duff);
                    continue;
                }
                strcpy(username, buf);
                

                while(1)        //循环得到UserData中存在的密保
                {
                    strcpy(duff, "---请输入密保(q to quit):");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    flag = mysql_repeat(&cm.mysql, "UserData", buf, 4);
                    if(flag == 1)
                    {
                        strcpy(duff, "---密保错误");
                        Write(cm.cfd, duff);
                        continue;
                    }
                }

                //账号密保均输入正确
                //接下来开始设置新的密码
                while(1)
                {
                    strcpy(duff, "---请输入新密码(q to quit):");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    strcpy(password, buf);
                    strcpy(duff, "---请再次输入密码(q to quit):");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    if(strcmp(buf, password) != 0)
                    {
                        strcpy(duff, "---两次密码输入不同");
                        Write(cm.cfd, duff);
                        continue;
                    }

                    //把新的密码更新到UserData中
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update UserData set password = \"%s\" where username = \"%s\"", \
                                                                        password, username);
                    rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                    if(rows != 0)
                    {
                        printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                        my_err("mysql_real_query error", __LINE__);
                    }
                    strcpy(duff, "---修改密码成功\n");
                    Write(cm.cfd, duff);
                    break;
                }
                break;
            }
        }
        else if(strcmp(buf, "q") == 0)    //关闭子线程
        {
            break;
        }
    }

    pthread_exit(0);
}

void *func_yonghu(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;
    
    int rows;
    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    pthread_t thid;

    int i, j;
    int choose;
    int flag;
    int shield_flag;

    char query_str[BUFSIZ];
    char buf[BUFSIZ];
    char temp[BUFSIZ];
    char now_time[BUFSIZ];
    char duff[BUFSIZ];

    int newsnum = 0;

    while(1)
    {
        //打印自己
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "------%s------\n", cm.username);
        Write(cm.cfd, temp);
        memset(temp, 0, sizeof(temp));
        //打印自己的未读消息news
        //得先找到自己的未读消息数newsnum
        newsnum = mysql_inquire_newsnum(&cm.mysql, cm.username, __LINE__);
        sprintf(temp, "------news(%d)(v to view)------\n", newsnum);
        Write(cm.cfd, temp);
        memset(temp, 0, sizeof(temp));
        //打印用户界面选项
        strcpy(duff, "[a] 好友列表\n[b] 添加好友\n[c] 群列表\n[d] 群选项\n[e] 列表管理\n[s] 刷新列表\n[q] 退出登陆\n");
        Write(cm.cfd, duff);
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "v") == 0)
        {
            while(1)
            {
                i = 1;
                j = 1;
                strcpy(duff, "---输入序号以处理消息(q to quit)\n");
                Write(cm.cfd, duff);
                //
                //打印出自己的未读消息列表
                //
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "select * from OffLineMes where touser = \"%s\"", cm.username);
                rows = mysql_real_query(&cm.mysql, temp, strlen(temp));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    memset(temp, 0, sizeof(temp));
                    sprintf(temp, "[%d]-<%s>-(%s)---%s\n", i++, row[0], row[1], row[3]);
                    Write(cm.cfd, temp);
                }
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    //打印出用户界面
                    break;
                }
                choose = atoi(buf);
                if(choose == 0)
                {
                    continue;
                }
                else if(choose >= i)
                {
                    strcpy(duff, "---输入超出范围，请重新输入:");
                    Write(cm.cfd, duff);
                    continue;
                }
                //再次遍历一遍自己的未读消息
                //找到自己选择的消息并处理
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "select * from OffLineMes where touser = \"%s\"", cm.username);
                rows = mysql_real_query(&cm.mysql, temp, strlen(temp));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    if(j++ == choose)
                    {
                        if(atoi(row[4]) == 1)       //加好友型消息
                        {
                            while(1)
                            {
                                strcpy(duff, "------同意(t)/拒绝(f)(q to quit)------\n");
                                Write(cm.cfd, duff);
                                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                                if(strcmp(buf, "t") == 0)
                                {
                                    //给自己一个回馈
                                    strcpy(duff, "---已接受\n");
                                    Write(cm.cfd, duff);

                                    //给加好友的人一个回馈
                                    sprintf(query_str, "insert into OffLineMes values\
                                    (\"%s\", \"%s\", \"%s\", \"%s接受了你的好友请求\", 2)", \
                                    get_time(now_time), row[2], row[1], row[2]);
                                    rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                                    if(rows != 0)
                                    {
                                        printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                                        my_err("mysql_real_query error", __LINE__);
                                    }
                                    memset(query_str, 0, sizeof(query_str));

                                    //给加好友的人newsnum+1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[1], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                                                                                    ++newsnum,              row[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把自己的newsnum-1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[2], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                                                                                    --newsnum,              row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把这条消息从自己的未读消息队列中抹去
                                    sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把好友加入自己的好友列表
                                    sprintf(query_str, "insert into %s values(\"%s\", \"1\")", row[2], row[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把自己加入对方好友列表
                                    sprintf(query_str, "insert into %s values(\"%s\", \"1\")", row[1], row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));


                                    break;
                                }
                                else if(strcmp(buf, "f") == 0)
                                {
                                    //给自己一个反馈
                                    strcpy(duff, "---已拒绝\n");
                                    Write(cm.cfd, duff);

                                    //给对方一个反馈
                                    sprintf(query_str, "insert into OffLineMes values\
                                    (\"%s\", \"%s\", \"%s\", \"%s拒绝了你的好友请求\", 2)", \
                                    get_time(now_time), row[2], row[1], row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把这条消息从OffLineMes中抹去
                                    sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //获得自己的newsnum-1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[2], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = \"%d\" where username = \"%s\"", \
                                                                                        --newsnum,              row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //获得对方的newsnum+1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[1], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = \"%d\" where username = \"%s\"", \
                                                                                        ++newsnum,              row[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    break;
                                }
                                else if(strcmp(buf, "q") == 0)
                                {
                                    break;
                                }
                                else
                                {
                                    strcpy(duff, "---请输入t/f/q---\n");
                                    Write(cm.cfd, duff);
                                    continue;   
                                }
                            }
                            break;
                        }
                        else if(atoi(row[4]) == 2)  //只读型消息
                        {
                            //获取自己的newsnum-1
                            newsnum = mysql_inquire_newsnum(&cm.mysql, row[2], __LINE__);
                            sprintf(query_str, "update UserData set newsnum = \"%d\" where username = \"%s\"", \
                                                                                --newsnum,              row[2]);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            memset(query_str, 0, strlen(query_str));

                            //从OffLineMes中移除这条消息
                            sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            memset(query_str, 0, strlen(query_str));
                            
                            break;
                        }
                        else if(atoi(row[4]) == 0)  //聊天型消息,进入与对方的聊天框
                        {
                            strcpy(cm.tousername, row[1]);
                            
                            //判断对方是否屏蔽自己
                            shield_flag = 1;
                            sprintf(query_str, "select num from %s where username = \"%s\"", \
                                                                cm.tousername,    cm.username);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            res2 = mysql_store_result(&cm.mysql);
                            if(res2 == NULL)
                            {
                                my_err("mysql_store_result error", __LINE__);
                            }
                            while(row2 = mysql_fetch_row(res2))
                            {
                                if(atoi(row2[0]) == 0)
                                {
                                    strcpy(temp, "---对方已将你屏蔽\n");
                                    Write(cm.cfd, temp);
                                    shield_flag = 0;
                                }
                            }
                            if(shield_flag == 0)
                            {
                                break;
                            }

                            //判断对方是否在线
                            sprintf(query_str, "select status from UserData where username = \"%s\"", cm.tousername);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            res2 = mysql_store_result(&cm.mysql);
                            if(res2 == NULL)
                            {
                                my_err("mysql_store_result error", __LINE__);
                            }
                            while(row2 = mysql_fetch_row(res2))
                            {
                                if(atoi(row2[0]) == 0)
                                {
                                    //进入留言线程
                                    if(pthread_create(&thid, NULL, func_liuyan, (void *)&cm) == -1)
                                    {
                                        my_err("pthread_create error", __LINE__);
                                    }
                                    pthread_join(thid, NULL);
                                }
                                else if(atoi(row2[0]) == 1)
                                {
                                    //进入聊天线程
                                    if(pthread_create(&thid, NULL, func_liaotian, (void *)&cm) == -1)
                                    {
                                        my_err("pthread_create error", __LINE__);
                                    }
                                    pthread_join(thid, NULL);
                                }
                            }

                            //更新自己的newsnum-1
                            newsnum = mysql_inquire_newsnum(&cm.mysql, cm.username, __LINE__);
                            sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                                                                            --newsnum,          cm.username);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                            //将OffLineMes中---
                            //---关于inuser = cm.tousername and touser = cm.username and type = 0的消息删除
                            sprintf(query_str, "delete from OffLineMes \
                            where inuser = \"%s\" and touser = \"%s\" and type = 0", \
                                            cm.tousername,      cm.username);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);


                            break;
                        }
                    }
                }
            }
        }
        else if(strcmp(buf, "a") == 0)
        {
            while(1)
            {
                //列出自己的好友
                memset(query_str, 0, sizeof(query_str));
                sprintf(query_str, "select * from %s", cm.username);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "select status from UserData where username = \"%s\"", row[0]);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res2 = mysql_store_result(&cm.mysql);
                    if(res2 == NULL)
                    {
                        my_err("mysql_real_query error", __LINE__);
                    } 
                    while(row2 = mysql_fetch_row(res2))
                    {
                        if(atoi(row2[0]) == 1)
                        {
                            memset(temp, 0, sizeof(temp));
                            sprintf(temp, "------%s---在线\n", row[0]);
                            Write(cm.cfd, temp);
                        }
                        else if(atoi(row2[0]) == 0)
                        {
                            memset(temp, 0, sizeof(temp));
                            sprintf(temp, "------%s---离线\n", row[0]);
                            Write(cm.cfd, temp);
                        }
                    }
                }

                //输入好友的用户名进入聊天界面
                strcpy(duff, "---输入好友的用户名进入聊天界面(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, cm.username, buf, 1);
                if(flag == 1)
                {
                    strcpy(duff, "---你没有此好友，请重新输入\n");
                    Write(cm.cfd, duff);
                    continue;
                }
                else if(flag == 0)
                {
                    strcpy(cm.tousername, buf);

                    //判断对方是否屏蔽自己
                    shield_flag = 1;
                    sprintf(query_str, "select num from %s where username = \"%s\"", \
                                                        cm.tousername,    cm.username);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res2 = mysql_store_result(&cm.mysql);
                    if(res2 == NULL)
                    {
                        my_err("mysql_store_result error", __LINE__);
                    }
                    while(row2 = mysql_fetch_row(res2))
                    {
                        if(atoi(row2[0]) == 0)
                        {
                            strcpy(temp, "---对方已将你屏蔽\n");
                            Write(cm.cfd, temp);
                            shield_flag = 0;
                        }
                    }
                    if(shield_flag == 0)
                    {
                        continue;
                    }

                    //判断对方是否在线
                    sprintf(query_str, "select status from UserData where username = \"%s\"", cm.tousername);
                    printf("%s\n", query_str);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res2 = mysql_store_result(&cm.mysql);
                    if(res2 == NULL)
                    {
                        printf("%d:error:%s\n", __LINE__, mysql_error(&cm.mysql));
                        my_err("mysql_store_result error", __LINE__);
                    }
                    while(row2 = mysql_fetch_row(res2))
                    {
                        if(atoi(row2[0]) == 0)
                        {
                            //进入留言线程
                            if(pthread_create(&thid, NULL, func_liuyan, (void *)&cm) == -1)
                            {
                                my_err("pthread_create error", __LINE__);
                            }
                            pthread_join(thid, NULL);
                        }
                        else if(atoi(row2[0]) == 1)
                        {
                            //进入聊天线程
                            if(pthread_create(&thid, NULL, func_liaotian, (void *)&cm) == -1)
                            {
                                my_err("pthread_create error", __LINE__);
                            }
                            pthread_join(thid, NULL);
                        }
                    }
                    break;
                }
            }
        }
        else if(strcmp(buf, "b") == 0)
        {
            strcpy(duff, "---添加好友(1)/添加群(2)(q to quit)---\n");
            Write(cm.cfd, duff);
            Read(cm.cfd, buf, sizeof(buf), __LINE__);
            if(strcmp(buf, "q") == 0)
            {
                continue;
            }
            else if(strcmp(buf, "1") == 0)
            {
                while(1)
                {
                    strcpy(duff, "---请输入用户名:");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                    if(flag == 0)
                    {
                        strcpy(duff, "---已发送好友请求\n");
                        Write(cm.cfd, duff);

                        //把这条消息发送到OffLineMes中
                        memset(query_str, 0, sizeof(query_str));
                        sprintf(query_str, "insert into OffLineMes values\
                        (\"%s\", \"%s\", \"%s\", \"%s向你发来好友请求\", \"1\")", \
                        get_time(now_time), cm.username, buf, cm.username);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                        //把对方newsnum+1
                        newsnum = mysql_inquire_newsnum(&cm.mysql, buf, __LINE__);
                        memset(query_str, 0, sizeof(query_str));
                        sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                                                                            ++newsnum,          buf);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    }
                    else if(flag == 1)
                    {
                        strcpy(duff, "---不存在此用户\n");
                        Write(cm.cfd, duff);
                        continue;
                    }
                }
            }
            else if(strcmp(buf, "2") == 0)
            {

            }
        }
        else if(strcmp(buf, "c") == 0)
        {
            if(pthread_create(&thid, NULL, func_group_list, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "d") == 0)
        {
            if(pthread_create(&thid, NULL, func_Group_options, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
        }
        else if(strcmp(buf, "e") == 0)
        {
            
        }
        else if(strcmp(buf, "q") == 0)//
        {
            //把用户的status设置为0
            sprintf(query_str, "update UserData set status = 0 where username = \"%s\"", cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            memset(query_str, 0, sizeof(query_str));
            break;
        }
        else if(strcmp(buf, "s") == 0)//
        {
            continue;
        }
    }

    pthread_exit(0);
}

void *func_liaotian(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    pthread_t thid;

    int newsnum;

    char buf[BUFSIZ];
    char query_str[BUFSIZ];
    char temp[BUFSIZ];
    char now_time[BUFSIZ];

    memset(buf, 0, sizeof(buf));
    memset(query_str, 0, sizeof(query_str));
    memset(temp, 0, sizeof(temp));


    sprintf(temp, "------%s(\"quit-exit\" to quit)------\n", cm.tousername);
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //给出好友选项参数
    //-hisdata  查看消息记录
    strcpy(temp, "------(-hisdata to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //-Friends_permissions  管理好友权限
    strcpy(temp, "------(-Friends_permissions to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));


    //先把对方发送过来的未读消息全部打印出来
    sprintf(query_str, "select * from OffLineMes where touser = \"%s\"", cm.username);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
        my_err("mysql_store_result error", __LINE__);
    while(row = mysql_fetch_row(res))
    {
        sprintf(temp, "<%s>-<%s>:%s\n", row[0], row[1], row[3]);
        Write(cm.cfd, temp);
        memset(temp, 0, sizeof(temp));
    }
    memset(query_str, 0, sizeof(query_str));

    //获得对方的套接字
    sprintf(query_str, "select cfd from UserData where username = \"%s\"", cm.tousername);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        cm.tocfd = atoi(row[0]);
    }

    //然后进入聊天
    //对方收到消息之后需要手动进入与自己的对话框才可以回复消息
    // printf("cm.cfd = %d\n", cm.cfd);
    // printf("cm.tocfd = %d\n", cm.tocfd);
    while(1)
    {
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "quit-exit") == 0)
        {
            break;
        }
        else if(strcmp(buf, "-hisdata") == 0)
        {
            //显示出历史消息记录
            sprintf(query_str, "select * from HisData \
            where inuser = \"%s\" and touser = \"%s\" or inuser = \"%s\" and touser = \"%s\"", \
                        cm.username, cm.tousername,                 cm.tousername, cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            res = mysql_store_result(&cm.mysql);
            if(res == NULL)
            {
                my_err("mysql_store_result error", __LINE__);
            }
            while(row = mysql_fetch_row(res))
            {
                sprintf(temp, "<%s>-<%s>-><%s>:%s\n", row[0], row[1], row[2], row[3]);
                Write(cm.cfd, temp);
            }
            continue;
        }
        else if(strcmp(buf, "-Friends_permissions") == 0)   //好友权限管理
        {
            if(pthread_create(&thid, NULL, func_Friends_permissions, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }

        sprintf(temp, "<%s>-<%s>:%s", get_time(now_time), cm.username, buf);
        Write(cm.tocfd, temp);

        //将读到的内容加入聊天记录HisData中
        sprintf(query_str, "insert into HisData values\
        (\"%s\", \"%s\", \"%s\", \"%s\")", \
        now_time, cm.username, cm.tousername, buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    }

    pthread_exit(0);
}

void *func_liuyan(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row;
    MYSQL_RES *res;

    pthread_t thid;

    int newsnum;

    char query_str[BUFSIZ];
    char buf[BUFSIZ];
    char now_time[BUFSIZ];
    char temp[BUFSIZ];

    memset(query_str, 0, sizeof(query_str));
    sprintf(query_str, "------%s(\"quit-exit\" to quit)------离线\n", cm.tousername);
    Write(cm.cfd, query_str);
    //给出好友选项参数
    //-hisdata  查看消息记录
    strcpy(temp, "------(-hisdata to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //-Friends_permissions  管理好友权限
    strcpy(temp, "------(-Friends_permissions to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));

    while(1)
    {
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "quit-exit") == 0)
        {
            break;        
        }
        else if(strcmp(buf, "-hisdata") == 0)
        {
            //显示出历史消息记录
            sprintf(query_str, "select * from HisData \
            where inuser = \"%s\" and touser = \"%s\" or inuser = \"%s\" and touser = \"%s\"", \
                        cm.username, cm.tousername,                 cm.tousername, cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            res = mysql_store_result(&cm.mysql);
            if(res == NULL)
            {
                my_err("mysql_store_result error", __LINE__);
            }
            while(row = mysql_fetch_row(res))
            {
                sprintf(temp, "<%s>-<%s>-><%s>:%s\n", row[0], row[1], row[2], row[3]);
                Write(cm.cfd, temp);
            }
            continue;
        }
        else if(strcmp(buf, "-Friends_permissions") == 0)   //好友权限管理
        {
            if(pthread_create(&thid, NULL, func_Friends_permissions, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }


        //把消息加入未读消息队列OffLineMes中
        sprintf(query_str, "insert into OffLineMes values\
        (\"%s\", \"%s\", \"%s\", \"%s\", \"0\")", \
        get_time(now_time), cm.username, cm.tousername, buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        memset(query_str, 0, sizeof(query_str));
        //给消息接收人的newsnum+1
        newsnum = mysql_inquire_newsnum(&cm.mysql, cm.tousername, __LINE__);
        sprintf(query_str, "update UserData set newsnum = \"%d\"", ++newsnum);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        memset(query_str, 0, sizeof(query_str));

        //把消息加入聊天记录HisData中
        sprintf(query_str, "insert into HisData values\
        (\"%s\", \"%s\", \"%s\", \"%s\")", \
        now_time, cm.username, cm.tousername, buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    }

    pthread_exit(0);
}


void *func_Friends_permissions(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row;
    MYSQL_RES *res;

    char temp[BUFSIZ];
    char buf[BUFSIZ];
    char query_str[BUFSIZ];


    //先看看好友是否已被屏蔽
    memset(query_str, 0, sizeof(query_str));
    sprintf(query_str, "select num from %s where username = \"%s\"", \
                                cm.username,            cm.tousername);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        if(atoi(row[0]) == 1)
        {
            while(1)
            {
                strcpy(temp, "[1] 屏蔽好友信息\n[2] 删除好友\n[q] 返回");
                Write(cm.cfd, temp);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "1") == 0)
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update %s set num = \"0\" where username = \"%s\"", \
                                            cm.username,                       cm.tousername);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    
                    sprintf(temp, "---已屏蔽好友:%s\n", cm.tousername);
                    Write(cm.cfd, temp);
                }
                else if(strcmp(buf, "2") == 0)
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "delete from %s where username = \"%s\"", \
                                                    cm.username,    cm.tousername);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    
                    sprintf(temp, "---已删除好友:%s\n", cm.tousername);
                    Write(cm.cfd, temp);
                }
                else if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                else
                {
                    strcpy(temp, "---输入不合规范，请输入1/2/q\n");
                    Write(cm.cfd, temp);
                    continue;
                }
            }
        }
        else if(atoi(row[0]) == 0)
        {
            while(1)
            {
                strcpy(temp, "[1] 屏蔽好友信息---解除\n[2] 删除好友\n[q] 返回\n");
                Write(cm.cfd, temp);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "1") == 0)
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update %s set num = \"1\" where username = \"%s\"", \
                                            cm.username,                       cm.tousername);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    
                    sprintf(temp, "---已解除屏蔽好友:%s\n", cm.tousername);
                    Write(cm.cfd, temp);
                }
                else if(strcmp(buf, "2") == 0)
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "delete from %s where username = \"%s\"", \
                                                    cm.username,    cm.tousername);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    
                    sprintf(temp, "---已删除好友:%s\n", cm.tousername);
                    Write(cm.cfd, temp);
                }
                else if(strcmp(buf, "q") == 0)
                {
                    break;
                }
            }
        }
    }

    pthread_exit(0);
}