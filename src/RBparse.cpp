#include "RBparse.h"
#include <iostream>
#include <sstream>
#include "parse.h"

void RBparse::passspace()
{
    while(cur < buff.size() && buff[cur] == ' ' && cur< buff.size()-1)
        ++cur;
}

std::string leftattr[20];

void RBparse::startparse()
{
    while (true)
    {
        std::cout<<"redbase> "<<std::flush;
        std::getline(std::cin, buff);
        switch(buff[cur])
        {
            case 'c':
                create();
                break;

            case 'd':
                drop();
                break;

            case 'l':
                load();
                break;

            case 'h':
                help();
                break;

            case 'p':
                Print();
                break;

            case 's':
                select();
                break;

            case 'i':
                insert();
                break;

            case 'u':
                update();
                break;
            case 'e':
                if((cur+3) < buff.size() && buff[cur+1] =='x' && buff[cur+2] == 'i' && buff[cur+3] == 't')
                {
                    std::cout<<"bye"<<std::endl;
                    return;
                }
        }
        cur = 0;
    }
}


void RBparse::create()
{
    cur+=6;
    passspace();
    if(buff[cur] == 't')
        createtable();
    else if(buff[cur] == 'i')
        createindex();
    else
        std::cout<<"commond error"<<std::endl;
}


void RBparse::drop()
{
    cur+=1;
    if(buff[cur] == 'e' )
    {
        cur+=5;
        passspace();
        Delete();
    }
    else
    {   cur+=3;
        passspace();
        if(buff[cur] == 't')
            droptable();
        else if(buff[cur] == 'i')
            dropindex();
        else
            std::cout<<"commond error"<<std::endl;
    }

}

void RBparse::createtable()
{
    AttrInfo attr [MAXATTRS];//最大属性个数40
    string attrname[MAXATTRS];
    int count = 0;
    cur+= 5;
    passspace();
    int temp = cur;
    while(buff[temp] != '(')
    {
        temp++;
    }
    string relname(buff.begin()+cur, buff.begin()+ temp);
    cur = temp;
    cur++;

    while (true)
    {
        int temp = cur;
        while(buff[temp] != ' ')
            temp++;
        attrname[count].assign(buff.begin()+cur, buff.begin()+ temp);
        attr[count].attrName = (char*)attrname[count].c_str();
        cur = temp;
        ++cur;

        temp =cur;
        while(buff[temp] != ' ' && buff[temp] != ',' && buff[temp] != ')')
            temp++;

        string type(buff.begin()+cur, buff.begin()+ temp);
        string a("string");
        string b("int");
        string c("float");
        if(type ==  a)
        {
            attr[count].attrType = STRING;
        }
        else if(type == b)
        {
            attr[count].attrType = INT;
        }
        else
        {
            attr[count].attrType = FLOAT;
        }

        cur = temp;

        if(buff[cur] == ')')
        {
            if(attr[count].attrType == INT)
                attr[count].attrLength = sizeof(int);
            else if(attr[count].attrType == FLOAT)
                attr[count].attrLength = sizeof(float);
            count++;
            break;
        }
        else if(buff[cur] == ' ')
        {
            cur++;
            temp = cur;
            while(buff[temp] != ',' && buff[temp] != ')')
                temp++;
            string len(buff.begin()+cur, buff.begin()+ temp);
            std::istringstream ss(len);
            ss >> attr[count].attrLength;
            cur = temp;
            if(buff[cur] == ')')
            {
                count++;
                break;
            }
        }
        else
        {
            if(attr[count].attrType == INT)
                attr[count].attrLength = sizeof(int);
            else if(attr[count].attrType == FLOAT)
                attr[count].attrLength = sizeof(float);
        }
        cur++;
        passspace();
        count++;
        if(buff[cur] == ')')
        {
            count++;
            break;
        }
    }
    int rc = psm.createTable(relname.c_str(),count, attr);
    if(rc == 0)
        std::cout<<"create table ok"<<std::endl;
    else
        std::cout<<"create table error"<<std::endl;
}

void RBparse::createindex()
{
    cur+=6;
    int temp =cur;
    while(buff[temp] != '(')
    {
        temp++;
    }
    string relname(buff.begin()+cur, buff.begin()+ temp);
    cur = temp;
    cur++;
    temp =cur;
    while(buff[temp] != ')')
    {
        temp++;
    }
    string attrname(buff.begin()+cur, buff.begin()+ temp);

    int rc;
    rc = psm.createIndex(relname.c_str(), attrname.c_str());

    if(rc == 0)
        std::cout<<"create index ok"<<std::endl;
    else
        std::cout<<"create index error"<<std::endl;

}


void RBparse::droptable()
{
    cur+=6;
    string relname(buff.begin()+cur, buff.end());
    int rc;
    rc = psm.dropTable(relname.c_str());

    if(rc == 0)
        std::cout<<"drop table ok"<<std::endl;
    else
        std::cout<<"drop table error"<<std::endl;
}


void RBparse::dropindex()
{
    cur+=6;
    int temp =cur;
    while(buff[temp] != '(')
    {
        temp++;
    }
    string relname(buff.begin()+cur, buff.begin()+ temp);
    cur = temp;
    cur++;
    temp =cur;
    while(buff[temp] != ')')
    {
        temp++;
    }
    string attrname(buff.begin()+cur, buff.begin()+ temp);

    int rc;
    rc = psm.dropIndex(relname.c_str(), attrname.c_str());
    if(rc == 0)
        std::cout<<"drop index ok"<<std::endl;
    else
        std::cout<<"drop index error"<<std::endl;
}

void RBparse::load()
{
    cur += 5;
    int temp =cur;
    while(buff[temp] != '(')
    {
        temp++;
    }
    string relname(buff.begin()+cur, buff.begin()+ temp);
    cur = temp;
    cur++;
    temp =cur;
    while(buff[temp] != ')')
    {
        temp++;
    }
    string filename(buff.begin()+cur, buff.begin()+ temp);

    int rc;
    rc = psm.load(relname.c_str(), filename.c_str());

    if(rc == 0)
        std::cout<<"load ok"<<std::endl;
    else
        std::cout<<"load error"<<std::endl;
}

void RBparse::help()
{
    if(buff.size()>4)
    {
        cur+= 5;
        string relname(buff.begin()+cur, buff.end()-1);
        psm.help((char*)relname.c_str());
    }
    else
    {
        psm.help();
    }

}

void RBparse::Print()
{
    cur+= 6;
    string relname(buff.begin()+cur, buff.end()-1);
    psm.print((char*)relname.c_str());
}

void RBparse::insert()
{
    int count = 0;
    Value values[MAXATTRS];
    cur+=12;
    int temp = cur;
    while(buff[temp] != ' ')
        temp++;
    string relname(buff.begin()+cur, buff.begin()+temp);
    cur = temp;
    passspace();
    cur+=6;
    passspace();
    cur++;
    while(true)
    {
        temp = cur;
        while(buff[temp] != ',' && buff[temp] != ')')
        {
            temp++;
        }
        string value(buff.begin()+cur, buff.begin()+temp);
        if(buff[cur] >= '0' && buff[cur]<= '9')
        {
            if(value.find('.') == std::string::npos)
            {
                std::istringstream out(value);
                int a = 0;
                out>>a;
                values[count].type = INT;
                int *tempptr = new int;
                memcpy(tempptr, &a, sizeof(int));
                values[count].data = tempptr;
            }
            else
            {
                std::istringstream out(value);
                float a = 0.0;
                out>>a;
                float* tempptr = new float;
                values[count].type = FLOAT;
                memcpy(tempptr, &a, sizeof(float));
                values[count].data = tempptr;
            }
        }
        else
        {
            values[count].type = STRING;
            values[count].data = (void*)value.c_str();
        }

        cur = temp;
        if(buff[temp] == ')')
        {
            count++;
            break;
        }
        cur++;
        passspace();
        count++;
    }
    int rc;
    rc = qlm.insert(relname.c_str(),count, values);
    if(rc == 0)
        std::cout<<"insert ok"<<std::endl;
    else
        std::cout<<"insert error"<<std::endl;
    for(int i = 0; i<count ; ++i)
    {
        if(values[i].type != STRING)
            delete values[i].data;
    }

}


void RBparse::select()
{
    RelAttr* attrs  = new RelAttr[MAXATTRS];
    int attrcount = 0;
    cur+=6;
    passspace();
    string name[MAXATTRS];
    while(true)
    {
        int temp = cur;
        while(buff[temp] != ' ' && buff[temp] != ',')
            temp++;
        name[attrcount].assign(buff.begin()+cur, buff.begin()+temp);
        attrs[attrcount].attrname = (char*)name[attrcount].c_str();
        attrs[attrcount].relname = NULL;
        attrcount++;
        cur =temp;
        if(buff[cur] == ',')
        {
            cur++;
            passspace();
        }
        else if(buff[cur] == ' ')
            break;

    }

    passspace();
    cur+=4;
    passspace();
    int temp = cur;
    while(buff[temp] != ' ' && temp < buff.size())
        temp++;
    string relationname(buff.begin()+cur, buff.begin()+temp);
    const char*r = relationname.c_str();

    cur= temp;
    //没有condition
    if(cur >= buff.size() -1)
    {
        qlm.select(attrcount,attrs, 1, &r, 0, NULL);
        delete attrs;
        return;
    }
    passspace();
    cur+=6;
    passspace();

    string cm(buff.begin()+cur, buff.end());
    int conditioncount;
    Condition* condition = new Condition[20];
    parsecondition(cm, condition, conditioncount);
    qlm.select(attrcount,attrs, 1, &r, conditioncount, condition);

    delete attrs;
    for(int i = 0; i<conditioncount ; ++i)
    {
        if(condition[i].rhsValue.type != STRING)
            delete condition[i].rhsValue.data;
    }
    delete condition;
    return;;

}

void RBparse::parsecondition(string con, Condition* &condition, int &count)
{
    count = 0;
    int curr = 0;
    while (true)
    {
        int temp = curr ;
        while(con[temp] != ' ')
            temp++;
        leftattr[count].assign(con.begin()+curr, con.begin()+temp);
        condition[count].lhsAttr.relname =NULL;
        condition[count].lhsAttr.attrname =(char*)leftattr[count].c_str();
        condition[count].bRhsIsAttr = false;
        curr = temp;
        while(con[curr] == ' ')
            curr++;
        temp = curr;
        while(con[temp] != ' ')
            temp++;
        string comp(con.begin()+curr, con.begin()+temp);
        if(comp.size() == 1)
        {
            string a(">");
            string b("<");
            string c("=");
            if(comp == a)
            {
                condition[count].op = GT_OP;
            }
            if(comp == b)
            {
                condition[count].op = LT_OP;
            }
            if(comp == c)
            {
                condition[count].op = EQ_OP;
            }
        }
        else
        {
            string a(">=");
            string b("<=");
            string c("!=");
            if(comp == a)
            {
                condition[count].op = GE_OP;
            }
            if(comp == b)
            {
                condition[count].op = LE_OP;
            }
            if(comp == c)
            {
                condition[count].op = NE_OP;
            }
        }
        curr = temp;
        while(con[curr] == ' ')
            curr++;
        temp = curr;
        while( con[temp] != ' ' && con[temp] != ']')
            temp++;

        string value(con.begin()+curr, con.begin()+temp);
        if(con[curr] >= '0' && con[curr]<= '9')
        {
            if(value.find('.') == std::string::npos)
            {
                std::istringstream out(value);
                int a = 0;
                out>>a;
                condition[count].rhsValue.type = INT;
                int *tempptr = new int;
                memcpy(tempptr, &a, sizeof(int));
                condition[count].rhsValue.data = tempptr;
            }
            else
            {
                std::istringstream out(value);
                float a = 0.0;
                out>>a;
                float* tempptr = new float;
                condition[count].rhsValue.type = FLOAT;
                memcpy(tempptr, &a, sizeof(float));
                condition[count].rhsValue.data = tempptr;
            }
        }
        else
        {
            condition[count].rhsValue.type = STRING;
            condition[count].rhsValue.data = (void*)value.c_str();
        }

        curr = temp;
        if(con[curr] == ']')
        {
            count ++;
            return;
        }
        while(con[curr] == ' ')
            curr++;
        curr+= 3;
        while(con[curr] == ' ')
            curr++;
        count++;
    }

}

void RBparse::Delete()
{
    cur += 4;
    passspace();
    Value* values = new Value [MAXATTRS];
    int temp = cur;
    while(buff[temp] != ' ')
        temp++;
    string relname(buff.begin()+cur, buff.begin()+temp);
    cur = temp;

    passspace();
    cur+=6;
    passspace();
    string cm(buff.begin()+cur, buff.end());
    int conditioncount;
    Condition* condition = new Condition[20];
    parsecondition(cm, condition, conditioncount);
    int rc = qlm.Delete(relname.c_str(), conditioncount, condition);
    if(rc == 0)
        std::cout<<"delete ok"<<std::endl;
    else
        std::cout<<"delete error"<<std::endl;
    for(int i = 0; i<conditioncount ; ++i)
    {
        if(condition[i].rhsValue.type != STRING)
            delete condition[i].rhsValue.data;
    }
    delete condition;
}


void RBparse::update()
{
    cur+= 6;
    passspace();
    int temp = cur;
    while(buff[temp] != ' ')
        temp++;
    string relname(buff.begin()+cur, buff.begin()+temp);
    cur = temp;
    cur++;
    passspace();
    cur+= 3;
    passspace();
    temp = cur;
    while(buff[temp] != ' ')
        temp++;
    string attr(buff.begin()+cur, buff.begin()+temp);
    RelAttr updateattr;
    updateattr.attrname = (char*)attr.c_str();
    updateattr.relname = NULL;

    cur = temp;
    passspace();
    cur++;
    passspace();
    temp = cur;
    while(buff[temp] != ' ')
        temp++;


    string value(buff.begin()+cur, buff.begin()+temp);
    Value rhsvalue;
    if(buff[cur] >= '0' && buff[cur]<= '9')
    {
        if(value.find('.') == std::string::npos)
        {
            std::istringstream out(value);
            int a = 0;
            out>>a;
            rhsvalue.type = INT;
            int *tempptr = new int;
            memcpy(tempptr, &a, sizeof(int));
            rhsvalue.data = tempptr;
        }
        else
        {
            std::istringstream out(value);
            float a = 0.0;
            out>>a;
            float* tempptr = new float;
            rhsvalue.type = FLOAT;
            memcpy(tempptr, &a, sizeof(float));
            rhsvalue.data = tempptr;
        }
    }
    else
    {
        rhsvalue.type = STRING;
        rhsvalue.data = (void*)value.c_str();
    }
    cur = temp;
    passspace();
    cur+=6;
    passspace();
    string cm(buff.begin()+cur, buff.end());
    int conditioncount;
    Condition* condition = new Condition[20];
    parsecondition(cm, condition, conditioncount);
    qlm.Update(relname.c_str(),updateattr, 1,updateattr, rhsvalue, conditioncount, condition);
    for(int i = 0; i<conditioncount ; ++i)
    {
        if(condition[i].rhsValue.type != STRING)
            delete condition[i].rhsValue.data;
    }
    delete condition;
    if(rhsvalue.type != STRING)
        delete rhsvalue.data;


}



