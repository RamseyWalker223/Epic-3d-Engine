#include <iostream>
#include <SDL2/SDL.h>
#include <vector>
#include <fstream>

const int screenW = 1440;
const int screenH = 900;
const int width = 1440;
const int height = 900;
float a = 2.50f;
float zNear = 1.0/tan(a/2.0);
float z_buff[height][width];
uint32_t screen[height][width];

struct color{
    //This is for defining colors.
    uint32_t r, g, b, a;
};

struct point{
    float x, y, z;
    bool operator==(const point& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct tri{
    int triangle[3];
};

struct mesh{
    std::vector<tri> obj;
    std::vector<point> points;
    float radius;
    color obj_color;
};

class plane{
    // This class is simply for representing the different clipping planes, the normal and d members are different parts of the simple clipping equation; Which is ax + by + cz + d = 0
    //The dist function calculates the distance of any point to the plane.
    public:
        point normal;
        float d;
        float dist(point &normal, point &p, float d){
            return  normal.x * p.x + normal.y * p.y + normal.z * p.z + d;
        }
        plane(int &i){
            switch(i){
                case 0:
                    this->normal.x = 0;
                    this->normal.y = -1.0/sqrt(1.0 + tan(a/2.0) * tan(a/2.0));
                    this->normal.z = tan(a/2.0)/sqrt(1 + (tan(a/2.0) * tan(a/2.0)));
                    this->d = 0;
                    break;
                case 1:
                    this->normal.x = 0;
                    this->normal.y = 1.0/sqrt(1.0 + tan(a/2.0) * tan(a/2.0));
                    this->normal.z = tan(a/2.0)/sqrt(1 + (tan(a/2.0) * tan(a/2.0)));
                    this->d = 0;
                    break;
                case 2:
                    this->normal.x = 1.0/sqrt(1.0 + tan(a/2.0) * tan(a/2.0));
                    this->normal.y = 0;
                    this->normal.z = tan(a/2.0)/sqrt(1 + (tan(a/2.0) * tan(a/2.0)));
                    this->d = 0;
                    break;
                case 3:
                    this->normal.x = -1.0/sqrt(1.0 + tan(a/2.0) * tan(a/2.0));
                    this->normal.y = 0;
                    this->normal.z = tan(a/2.0)/sqrt(1 + (tan(a/2.0) * tan(a/2.0)));
                    this->d = 0;
                    break;
                case 4:
                    this->normal.x = 0;
                    this->normal.y = 0;
                    this->normal.z = 1;
                    this->d = zNear;
                    break;
            }
        }
};

bool is_top_left(int &x0, int &y0, int &x1, int &y1){
    bool is_top_edge = (y1 - y0) == 0 && (x1 - x0) > 0;
    bool is_left_edge = y1 - y0 < 0;
    return is_left_edge || is_top_edge;
}

int edge_cross(int &x0, int &y0, int &x1, int &y1, int &x2, int &y2){
    return ((x1 - x0) * (y2 - y0)) - ((x2 - x0) * (y1 - y0));
}

void line(int &x0, int &y0, int &x1, int &y1){
    if(abs(x1 - x0) > abs(y1 - y0)){
        if(x0 > x1){
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        int dx = x1 - x0;
        int dy = y1 - y0;
        int dir = (dy < 0) ? -1 : 1;
        dy = dy * dir;
        if(dx != 0){
            int y = y0;
            int p = (2 * dy) - dx;
            for(int i = 0; i < dx + 1; i++){
                if(y >= 0 && y < height && x0 + i >= 0 && x0 + i < width){
                    screen[y][x0 + i] = 0xFFFF0000;
                }
                if(p >= 0){
                    y += dir;
                    p = p - 2 * dx;
                }
                p = p + 2 * dy;
            }
        }
    } else {
        if(y0 > y1){
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        int dx = x1 - x0;
        int dy = y1 - y0;
        int dir = (dx < 0) ? -1 : 1;
        dx = dx * dir;
        if(dy != 0){
            int x = x0;
            int p = (2 * dx) - dy;
            for(int i = 0; i < dy + 1; i++){
                if(y0 + i >= 0 && y0 + i < height && x >= 0 && x < width){
                    screen[y0 + i][x] = 0xFFFF0000;
                }
                if(p >= 0){
                    x += dir;
                    p = p - 2*dy;
                }
                p = p + 2 * dx;
            }
        }
    }
}

void fill(int &x0, int &y0, int &x1, int &y1, int &x2, int &y2, float &z0, float &z1, float &z2){
    //First we sort the points of the triangle into clockwise orientation
    if(edge_cross(x0, y0, x1, y1, x2, y2) < 0){
        std::swap(x1, x2);
        std::swap(y1, y2);
        std::swap(z1, z2);
    }

    int maxX = std::max(std::max(x0, x1), x2);
    int maxY = std::max(std::max(y0, y1), y2);
    int minX = std::min(std::min(x0, x1), x2);
    int minY = std::min(std::min(y0, y1), y2);

    maxX = std::max(0, std::min(maxX, width - 1));
    maxY = std::max(0, std::min(maxY, height - 1));
    minX = std::min(width - 1, std::max(0, minX));
    minY = std::min(height - 1, std::max(0, minY));

    int area = edge_cross(x0, y0, x1, y1, x2, y2);

    int bias0 = is_top_left(x0, y0, x1, y1) ? 0 : -1;
    int bias1 = is_top_left(x1, y1, x2, y2) ? 0 : -1;
    int bias2 = is_top_left(x2, y2, x0, y0) ? 0 : -1;

    int w0_row = edge_cross(x0, y0, x1, y1, minX, minY) + bias0;
    int w1_row = edge_cross(x1, y1, x2, y2, minX, minY) + bias1;
    int w2_row = edge_cross(x2, y2, x0, y0, minX, minY) + bias2;

    int dw0c = y0 - y1;
    int dw1c = y1 - y2;
    int dw2c = y2 - y0;

    int dw0r = x1 - x0;
    int dw1r = x2 - x1;
    int dw2r = x0 - x2;

    //Create the colors: (Temporary)
    color colors[3] = {
        {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF },
        {.r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF },
        {.r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF }
    };

    for(int y = minY; y <= maxY; y++){
        int w0 = w0_row;
        int w1 = w1_row;
        int w2 = w2_row;
        for(int x = minX; x <= maxX; x++){

            if((w0 >= 0 && w1 >= 0 && w2 >= 0)){

                float A = (float)edge_cross(x, y, x0, y0, x1, y1)/(float)area;
                float B = (float)edge_cross(x, y, x1, y1, x2, y2)/(float)area;
                float Y = (float)edge_cross(x, y, x2, y2, x0, y0)/(float)area;

                float tri_Z = (A * z2) + (B * z0) + (Y * z1);


                if(tri_Z < z_buff[y][x]){
                    int a = (A * colors[0].a) + (B * colors[1].a) + (Y * colors[2].a);
                    int r = (A * colors[0].r) + (B * colors[1].r) + (Y * colors[2].r);
                    int g = (A * colors[0].g) + (B * colors[1].g) + (Y * colors[2].g);
                    int b = (A * colors[0].b) + (B * colors[1].b) + (Y * colors[2].b);

                    uint32_t interp_color = 0x00000000;
                    
                    interp_color = (interp_color | a) << 8;
                    interp_color = (interp_color | b) << 8;
                    interp_color = (interp_color | g) << 8;
                    interp_color = (interp_color | r);

                    screen[y][x] = interp_color;
                    z_buff[y][x] = tri_Z;
                }
            }
            w0 += dw0c;
            w1 += dw1c;
            w2 += dw2c;
        }
        w0_row += dw0r;
        w1_row += dw1r;
        w2_row += dw2r;
    }
}

void parse(const std::string &dataline, char delimeter, std::vector<std::string> &outputs, size_t size){
            size_t start;
            size_t end {0};
            size_t i {0};
            while(((start = dataline.find_first_not_of(delimeter, end)) != std::string::npos) && (i < size)){
                end = dataline.find(delimeter, start);
                outputs.emplace_back(dataline.substr(start, (end - start)));
                i++;
            }
        }

class object{
    public:
        mesh obj;
        point center;
        mesh import(std::string file, point &scale, point &origin){
            mesh object;
            int maxX;
            int minX;
            int maxY;
            int minY;
            int maxZ;
            int minZ;
            std::ifstream lines;
            lines.open(file);
            float cx;
            float cy;
            float cz;
            bool points_finished = false;
            std::string line;
            while(getline(lines, line)){
                if(line[0] == 'v' && line[1] == ' '){
                    std::vector<std::string> points;
                    parse(line, ' ', points, 4);
                    point p;
                    p.x = std::stof(points[1]) * scale.x;
                    p.y = std::stof(points[2]) * scale.y;
                    p.z = std::stof(points[3]) * scale.z;
                    object.points.push_back(p);
                    if (object.points.size() == 1) {
                        maxX = minX = maxY = minY = maxZ = minZ = 0;
                    } else {
                        if (p.x > object.points[maxX].x) maxX = object.points.size() - 1;
                        if (p.x < object.points[minX].x) minX = object.points.size() - 1;
                        if (p.y > object.points[maxY].y) maxY = object.points.size() - 1;
                        if (p.y < object.points[minY].y) minY = object.points.size() - 1;
                        if (p.z > object.points[maxZ].z) maxZ = object.points.size() - 1;
                        if (p.z < object.points[minZ].z) minZ = object.points.size() - 1;
                    }
                }
                if(line[0] == 'f'){
                    if(!points_finished){
                        cx = (object.points[maxX].x + object.points[minX].x)/2.0;
                        cy = (object.points[maxY].y + object.points[minY].y)/2.0;
                        cz = (object.points[maxZ].z + object.points[minZ].z)/2.0;
                    }
                    tri triangle;
                    std::vector<std::string> points;
                    parse(line, ' ', points, 4);
                    for(int i = 1; i < 4; i++){
                        for(char c: points[i]){
                            if(c == '/'){
                                std::vector<std::string> poo;
                                parse(points[i], '/', poo, 3);
                                points[i] = poo[0];
                                break;
                            }
                        }
                        triangle.triangle[i - 1] = std::stoi(points[i]) - 1;
                    }
                    object.obj.push_back(triangle);
                    points_finished = true;
                }
            }
            object.radius = 0;
            for(int i = 0; i < object.points.size(); i++){
                object.points[i].x = (object.points[i].x - cx) + origin.x;
                object.points[i].y = (object.points[i].y - cy) + origin.y;
                object.points[i].z = (object.points[i].z - cz) + origin.z;
                float dist = sqrt((object.points[i].x - origin.x) * (object.points[i].x - origin.x) + (object.points[i].y - origin.y) * (object.points[i].y - origin.y) + (object.points[i].z - origin.z) * (object.points[i].z - origin.z));
                object.radius = (dist > object.radius) ? dist : object.radius;
            }
            return object;
        }
        mesh cull(mesh object){
            //This function is kind of useless, as most 3d objects dont define their faces in a consistent winding order so that culling can work. But it still works for some of the object files.
            for(int i = 0; i < object.obj.size(); i++){
                point normal;
                normal.x = (object.points[object.obj[i].triangle[2]].y - object.points[object.obj[i].triangle[0]].y) * (object.points[object.obj[i].triangle[1]].z - object.points[object.obj[i].triangle[0]].z) - (object.points[object.obj[i].triangle[2]].z - object.points[object.obj[i].triangle[0]].z) * (object.points[object.obj[i].triangle[1]].y - object.points[object.obj[i].triangle[0]].y);
                normal.y = (object.points[object.obj[i].triangle[2]].z - object.points[object.obj[i].triangle[0]].z) * (object.points[object.obj[i].triangle[1]].x - object.points[object.obj[i].triangle[0]].x) - (object.points[object.obj[i].triangle[2]].x - object.points[object.obj[i].triangle[0]].x) * (object.points[object.obj[i].triangle[1]].z - object.points[object.obj[i].triangle[0]].z);
                normal.z = (object.points[object.obj[i].triangle[2]].x - object.points[object.obj[i].triangle[0]].x) * (object.points[object.obj[i].triangle[1]].y - object.points[object.obj[i].triangle[0]].y) - (object.points[object.obj[i].triangle[2]].y - object.points[object.obj[i].triangle[0]].y) * (object.points[object.obj[i].triangle[1]].x - object.points[object.obj[i].triangle[0]].x);
                if((normal.x * -1 * object.points[object.obj[i].triangle[0]].x) + (normal.y * -1 * object.points[object.obj[i].triangle[0]].y) + (normal.z * -1 * object.points[object.obj[i].triangle[0]].z) >= 0){
                    object.obj.erase(object.obj.begin() + i);
                    i--;
                }
            }
            return object;
        }
        mesh clip(mesh object, point &center){
            //For this function and the culling and project function, we can't take in a reference because we would then be modifying them in a way that would ruin the file for world space.
            //So we have to take in a copy and return the new mesh.
            //It would probably be a good optimization to merge these three functions into one for optimization or only copy it once and then pass a reference so that we dont end up making up to 3 copies, but I'm willing to make that memory sacrifice for better readability.
            for(int i = 0; i < 5; i++){
                plane clipper(i);
                float clipped = clipper.dist(clipper.normal, center, clipper.d);
                if((clipped < -1*object.radius)){
                    mesh clear;
                    return clear;
                } else if(abs(clipped) < object.radius){
                    for(int j = 0; j < object.obj.size(); j++){
                        int front[2];
                        int back[2];
                        int count = 0;
                        int bCount = 0;
                        for(int k = 0; k < 3; k++){
                            if(clipper.dist(clipper.normal, object.points[object.obj[j].triangle[k]], clipper.d) > 0){
                                if(count == 2){
                                    count++;
                                    break;
                                }
                                front[count] = k;
                                count++;
                            } else {
                                if(bCount == 2){
                                    break;
                                }
                                back[bCount] = k;
                                bCount++;
                            }
                        }
                        if(count == 3){
                            continue;
                        } else if(count == 2){
                            point B = object.points[object.obj[j].triangle[back[0]]];
                            tri tri1;
                            tri tri2;
                            tri1.triangle[0] = object.obj[j].triangle[front[0]];
                            tri1.triangle[1] = object.obj[j].triangle[front[1]];
                            tri1.triangle[2] = object.points.size();
                            
                            tri2.triangle[0] = object.points.size();
                            tri2.triangle[1] = object.obj[j].triangle[front[1]];
                            tri2.triangle[2] = object.points.size() + 1;

                            object.obj.erase(object.obj.begin() + j);
                            object.obj.insert(object.obj.begin() + j, tri2);
                            object.obj.insert(object.obj.begin() + j, tri1);

                            object.points.emplace_back();
                            object.points.emplace_back();

                            float t = (-1*clipper.d - (clipper.normal.x * object.points[tri1.triangle[0]].x + clipper.normal.y * object.points[tri1.triangle[0]].y + clipper.normal.z * object.points[tri1.triangle[0]].z))/(clipper.normal.x * (B.x - object.points[tri1.triangle[0]].x) + clipper.normal.y * (B.y - object.points[tri1.triangle[0]].y) + clipper.normal.z * (B.z - object.points[tri1.triangle[0]].z));
                            object.points[tri1.triangle[2]].x = object.points[tri1.triangle[0]].x + t*(B.x - object.points[tri1.triangle[0]].x);
                            object.points[tri1.triangle[2]].y = object.points[tri1.triangle[0]].y + t*(B.y - object.points[tri1.triangle[0]].y);
                            object.points[tri1.triangle[2]].z = object.points[tri1.triangle[0]].z + t*(B.z - object.points[tri1.triangle[0]].z);

                            j++;

                            t = (-1*clipper.d - (clipper.normal.x * object.points[tri2.triangle[1]].x + clipper.normal.y * object.points[tri2.triangle[1]].y + clipper.normal.z * object.points[tri2.triangle[1]].z))/(clipper.normal.x * (B.x - object.points[tri2.triangle[1]].x) + clipper.normal.y * (B.y - object.points[tri2.triangle[1]].y) + clipper.normal.z * (B.z - object.points[tri2.triangle[1]].z));
                            object.points[tri2.triangle[2]].x = object.points[tri2.triangle[1]].x + t*(B.x - object.points[tri2.triangle[1]].x);
                            object.points[tri2.triangle[2]].y = object.points[tri2.triangle[1]].y + t*(B.y - object.points[tri2.triangle[1]].y);
                            object.points[tri2.triangle[2]].z = object.points[tri2.triangle[1]].z + t*(B.z - object.points[tri2.triangle[1]].z);
                        
                        } else if(count == 1){
                            point B = object.points[object.obj[j].triangle[back[0]]];
                            point B2 = object.points[object.obj[j].triangle[back[1]]];
                            object.obj[j].triangle[back[0]] = object.points.size();
                            object.obj[j].triangle[back[1]] = object.points.size() + 1;
                            object.points.emplace_back();
                            object.points.emplace_back();

                            float t = (-1*clipper.d - (clipper.normal.x * object.points[object.obj[j].triangle[front[0]]].x + clipper.normal.y * object.points[object.obj[j].triangle[front[0]]].y + clipper.normal.z * object.points[object.obj[j].triangle[front[0]]].z))/(clipper.normal.x * (B.x - object.points[object.obj[j].triangle[front[0]]].x) + clipper.normal.y * (B.y - object.points[object.obj[j].triangle[front[0]]].y) + clipper.normal.z * (B.z - object.points[object.obj[j].triangle[front[0]]].z));

                            object.points[object.obj[j].triangle[back[0]]].x = object.points[object.obj[j].triangle[front[0]]].x + t*(B.x - object.points[object.obj[j].triangle[front[0]]].x);
                            object.points[object.obj[j].triangle[back[0]]].y = object.points[object.obj[j].triangle[front[0]]].y + t*(B.y - object.points[object.obj[j].triangle[front[0]]].y);
                            object.points[object.obj[j].triangle[back[0]]].z = object.points[object.obj[j].triangle[front[0]]].z + t*(B.z - object.points[object.obj[j].triangle[front[0]]].z);

                            t = (-1*clipper.d - (clipper.normal.x * object.points[object.obj[j].triangle[front[0]]].x + clipper.normal.y * object.points[object.obj[j].triangle[front[0]]].y + clipper.normal.z * object.points[object.obj[j].triangle[front[0]]].z))/(clipper.normal.x * (B2.x - object.points[object.obj[j].triangle[front[0]]].x) + clipper.normal.y * (B2.y - object.points[object.obj[j].triangle[front[0]]].y) + clipper.normal.z * (B2.z - object.points[object.obj[j].triangle[front[0]]].z));

                            object.points[object.obj[j].triangle[back[1]]].x = object.points[object.obj[j].triangle[front[0]]].x + t*(B2.x - object.points[object.obj[j].triangle[front[0]]].x);
                            object.points[object.obj[j].triangle[back[1]]].y = object.points[object.obj[j].triangle[front[0]]].y + t*(B2.y - object.points[object.obj[j].triangle[front[0]]].y);
                            object.points[object.obj[j].triangle[back[1]]].z = object.points[object.obj[j].triangle[front[0]]].z + t*(B2.z - object.points[object.obj[j].triangle[front[0]]].z);
                        } else {
                            object.obj.erase(object.obj.begin() + j);
                            j--;
                        }
                    }
                }
            }
            
            return object;
        }
        mesh project(mesh object){
            for(int i = 0; i < object.points.size(); i++){
                object.points[i].x = object.points[i].x/(object.points[i].z * tan(a/2.0));
                object.points[i].y = (object.points[i].y/(object.points[i].z * tan(a/2.0)))/((float)height/(float)width);
            }
            return object;
        }
        void draw_fill(mesh object){
            for(int b = 0; b < object.obj.size(); b++){
                int x0 = (int)round((object.points[object.obj[b].triangle[0]].x * width) + width/2.0) - 1;
                int y0 = (int)round((object.points[object.obj[b].triangle[0]].y * height) + height/2.0) - 1;
                int x1 = (int)round((object.points[object.obj[b].triangle[1]].x * width) + width/2.0) - 1;
                int y1 = (int)round((object.points[object.obj[b].triangle[1]].y * height) + height/2.0) - 1;
                int x2 = (int)round((object.points[object.obj[b].triangle[2]].x * width) + width/2.0) - 1;
                int y2 = (int)round((object.points[object.obj[b].triangle[2]].y * height) + height/2.0) - 1;
                float z0 = object.points[object.obj[b].triangle[0]].z;
                float z1 = object.points[object.obj[b].triangle[1]].z;
                float z2 = object.points[object.obj[b].triangle[2]].z;
                fill(x0, y0, x1, y1, x2, y2, z0, z1, z2);
            }
        }
        void draw_wire(mesh object){
            for(int b = 0; b < object.obj.size(); b++){
                tri triangle = object.obj[b];
                for(int i = 0; i < 3; i++){
                    int ly = (int)round((object.points[triangle.triangle[i]].y * height) + height/2.0) - 1;
                    int lx = (int)round((object.points[triangle.triangle[i]].x * width) + width/2.0) - 1;
                    if(i > 0){
                        int fx = (int)round((object.points[triangle.triangle[i - 1]].x * width) + width/2.0) - 1;
                        int fy = (int)round((object.points[triangle.triangle[i - 1]].y * height) + height/2.0) - 1;
                        line(fx, fy, lx, ly);
                    } else {
                        int fx = (int)round((object.points[triangle.triangle[2]].x * width) + width/2.0) - 1;
                        int fy = (int)round((object.points[triangle.triangle[2]].y * height) + height/2.0) - 1;
                        line(fx, fy, lx, ly);
                    }
                }
            }
        }
        void circle(mesh &object, float t0, char c){
            float cosine = cos(t0);
            float sine = sin(t0);
            float matrix[3][3]; 
            if(c == 'x'){
                matrix[0][0] = 1.0; 
                matrix[0][1] = 0.0;           
                matrix[0][2] = 0.0;
                matrix[1][0] = 0.0; 
                matrix[1][1] = cosine;        
                matrix[1][2] = -1 * sine;
                matrix[2][0] = 0.0; 
                matrix[2][1] = sine;          
                matrix[2][2] = cosine;
            } else {
                matrix[0][0] = cosine;  
                matrix[0][1] = 0.0;       
                matrix[0][2] = sine;
                matrix[1][0] = 0.0;     
                matrix[1][1] = 1.0;       
                matrix[1][2] = 0.0;
                matrix[2][0] = -1 * sine;   
                matrix[2][1] = 0.0;       
                matrix[2][2] = cosine;
            }
            for(int i = 0; i < object.points.size(); i++){
                float x = object.points[i].x * matrix[0][0] + object.points[i].y * matrix[1][0] + object.points[i].z * matrix[2][0];
                float y = object.points[i].x * matrix[0][1] + object.points[i].y * matrix[1][1] + object.points[i].z * matrix[2][1];
                float z = object.points[i].x * matrix[0][2] + object.points[i].y * matrix[1][2] + object.points[i].z * matrix[2][2];
                object.points[i].x = x;
                object.points[i].y = y;
                object.points[i].z = z;
            }
            //return object;
        }
        void circle(point &vector, float t0, char c){
            float cosine = cos(t0);
            float sine = sin(t0);
            float matrix[3][3]; 
            if(c == 'x'){
                matrix[0][0] = 1.0; 
                matrix[0][1] = 0.0;           
                matrix[0][2] = 0.0;
                matrix[1][0] = 0.0; 
                matrix[1][1] = cosine;        
                matrix[1][2] = -1 * sine;
                matrix[2][0] = 0.0; 
                matrix[2][1] = sine;          
                matrix[2][2] = cosine;
            } else {
                matrix[0][0] = cosine;  
                matrix[0][1] = 0.0;       
                matrix[0][2] = sine;
                matrix[1][0] = 0.0;     
                matrix[1][1] = 1.0;       
                matrix[1][2] = 0.0;
                matrix[2][0] = -1 * sine;   
                matrix[2][1] = 0.0;       
                matrix[2][2] = cosine;
            }
            float x = vector.x * matrix[0][0] + vector.y * matrix[1][0] + vector.z * matrix[2][0];
            float y = vector.x * matrix[0][1] + vector.y * matrix[1][1] + vector.z * matrix[2][1];
            float z = vector.x * matrix[0][2] + vector.y * matrix[1][2] + vector.z * matrix[2][2];
            vector.x = x;
            vector.y = y;
            vector.z = z;
            //return vector;
        }
        void rotate(mesh &object, float t0, point vector, point center){
            //Normalize the vector
            float l = sqrt((vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z));
            float ux = vector.x/l;
            float uy = vector.y/l;
            float uz = vector.z/l;
            float uxuy = ux * uy;
            float uxuz = ux * uz;
            float uyuz = uy * uz;
            float cos1 = 1.0 - cos(t0);
            float cosine = cos(t0);
            float sine = sin(t0);
            float matrix[3][3] = {
                                    {((ux * ux)*cos1 + cosine), ((uxuy*cos1) - uz*sine), ((uxuz*cos1) + uy*sine)},
                                    {(uxuy*cos1 + uz*sine), ((uy*uy)*cos1 + cosine), (uyuz*cos1 - ux*sine)},
                                    {(uxuz*cos1 - uy*sine), (uyuz*cos1 + ux*sine), ((uz * uz)*cos1 + cosine)}
                                };
            for(int i = 0; i < object.points.size(); i++){
                    //Move it to the origin
                    object.points[i].x -= center.x;
                    object.points[i].y -= center.y;
                    object.points[i].z -= center.z;
                    //Rotate it
                    float x = object.points[i].x * matrix[0][0] + object.points[i].y * matrix[1][0] + object.points[i].z * matrix[2][0];
                    float y = object.points[i].x * matrix[0][1] + object.points[i].y * matrix[1][1] + object.points[i].z * matrix[2][1];
                    float z = object.points[i].x * matrix[0][2] + object.points[i].y * matrix[1][2] + object.points[i].z * matrix[2][2];
                    object.points[i].x = x;
                    object.points[i].y = y;
                    object.points[i].z = z;
                    //Move it back
                    object.points[i].x += center.x;
                    object.points[i].y += center.y;
                    object.points[i].z += center.z;
            }
            //return object;
        }
        void tranZ(mesh &object, float dist, bool neg){
            for(int i = 0; i < object.points.size(); i++){
                if(neg){
                    object.points[i].z -= dist;
                } else {
                    object.points[i].z += dist;
                }
            }
            //return object;
        }
        void tranY(mesh &object, float dist, bool neg){
                for(int i = 0; i < object.points.size(); i++){
                if(neg){
                    object.points[i].y -= dist;
                } else {
                    object.points[i].y += dist;
                }
            }
            //return object;
        }
        void tranX(mesh &object, float dist, bool neg){
            for(int i = 0; i < object.points.size(); i++){
                if(neg){
                    object.points[i].x -= dist;
                } else {
                    object.points[i].x += dist;
                }
            }
            //return object;
        }
};

int main(){
    std::string file;
    std::cout << "Enter the 3d object file name:\n";
    std::cin >> file;
    if(file.length() < 5 || file.substr(file.length() - 4) != ".obj"){
        file+=".obj";
    }
    file = "./objects/" + file;
    //Create object
    object shape;
    shape.center.x = 0.0f;
    shape.center.y = 0.0f;
    shape.center.z = 30.0f;
    point scale;
    scale.x = 1.0f;
    scale.y = 1.0f;
    scale.z = 1.0f;
    shape.obj = shape.import(file, scale, shape.center);

    //Create Vector for rotation of object
    point vector;
    vector.x = 1.0f;
    vector.y = 1.0f;
    vector.z = 1.0f;
    

    //Set up window
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_Event e;
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Rect r{10, 10, 250, 250};
    SDL_CreateWindowAndRenderer(screenW, screenH, 0, &window, &renderer);
    SDL_RenderSetLogicalSize(renderer, width, height);

    //Initialize the screen texture
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    //Mouse setup
    SDL_SetRelativeMouseMode(SDL_TRUE);
    
    //Default to not in wireframe mode
    bool wireframe = false;

    //Game loop
    bool run = true;
    
    while(run){
        //Reset the screen array
        memset(screen, 0, sizeof(screen));
        //White = 0xFFFFFFFF
        

        //Rotate the object along a custom axis
        //shape.rotate(shape.obj, 0.0174533, vector, shape.center);


        //Cull it, clip it, project it and draw it to screen array:
        if(wireframe){
            //For drawing a wireframe
            shape.draw_wire(shape.project(shape.clip(shape.obj, shape.center)));
        } else {
            //Reset the z buffer
            for (int i = 0; i < height; i++){
                for (int j = 0; j < width; j++){
                    z_buff[i][j] = std::numeric_limits<float>::max();
                }
            }
            //For filling the triangles
            shape.draw_fill(shape.project(shape.clip(shape.obj, shape.center)));
        }
        

        //Event handling
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                run = false;
            }
            if(e.type == SDL_KEYDOWN){
                switch(e.key.keysym.sym){
                    case SDLK_w:
                        shape.tranZ(shape.obj, 1, true);
                        shape.center.z -= 1;
                        break;
                    case SDLK_s:
                        shape.tranZ(shape.obj, 1, false);
                        shape.center.z += 1;
                        break;
                    case SDLK_a:
                        shape.tranX(shape.obj, 1, false);
                        shape.center.x += 1;
                        break;
                    case SDLK_d:
                        shape.tranX(shape.obj, 1, true);
                        shape.center.x -= 1;
                        break;
                    case SDLK_q:
                        wireframe = !wireframe;
                        break;
                    case SDLK_LSHIFT:
                        shape.tranY(shape.obj, 1, true);
                        shape.center.y -= 1;
                        break;
                    case SDLK_SPACE:
                        shape.tranY(shape.obj, 1, false);
                        shape.center.y += 1;
                        break;
                    case SDLK_ESCAPE:
                        run = false;
                }
            }
            if(e.type == SDL_MOUSEMOTION){
                int dx = e.motion.xrel;
                int dy = e.motion.yrel;
                float theta = ((float)(dx)*1.25*M_PI/(float)width);
                float beta = (-1 * (float)(dy)*1.25*M_PI/(float)height);
                shape.circle(shape.obj, theta, 'y');
                shape.circle(vector, theta, 'y');
                shape.circle(shape.center, theta, 'y');
                shape.circle(shape.obj, beta, 'x');
                shape.circle(vector, beta, 'x');
                shape.circle(shape.center, beta, 'x');
            }
        }
    //Update the texture
    SDL_UpdateTexture(texture, NULL, screen, width * sizeof(uint32_t));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    
    //Draw to screen
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
    }
    
    return 0;
}