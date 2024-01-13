#ifndef CGLWIDGET_H
#define CGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMap>
#include <QVector3D>

struct sBufferWithColor {
    int mode;
    QOpenGLBuffer vbo;
    QVector4D lineColor;
};

struct sPlane {
    sPlane(QVector3D _n, QVector3D _p): n(_n), p(_p){}
    QVector3D n; //plane normal
    QVector3D p; //point on plane
};

class cGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit cGLWidget(QWidget *parent = nullptr);

    void setLightPos(const QVector3D lp);

    float scale(float from_min, float from_max, float to_min, float to_max, float val);

    QTimer *mTimer;

    QVector<GLfloat> mPoints;

    void updateProjection(bool perspectiveProjection);
    void topView();
    void leftView();
    void rightView();
    void frontView();
    QVector3D lightPos() const;

    void setLightColor(const QColor color);
    QColor lightColor();

    void addToVBO(const std::vector<GLfloat>& buffer, int mode=GL_LINES, QVector4D lineColor=QVector4D(1,1,1,1));
    void addToVBO(const QString& name, const std::vector<GLfloat>& buffer, int mode=GL_LINES, QVector4D lineColor=QVector4D(1,1,1,1));
    void clearVBOs();

    void destroyVBO(const QString &name);

    bool worldToScreen(const QVector3D &pw, int& x, int& y);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent *me);
    void mouseReleaseEvent(QMouseEvent *me);

    void mouseMoveEvent(QMouseEvent *me);
    void keyPressEvent(QKeyEvent *ke);
    void keyReleaseEvent(QKeyEvent *ke);

private:

    QMap<int, bool> mKeyDown;

    bool mPerspectiveProjection = true;

    QMap<QString, sBufferWithColor> mNamedVBOs;
    std::vector<sBufferWithColor> mVBOs;

    QOpenGLTexture *mTexture=nullptr;
    QOpenGLTexture *mTextureHeightNormal=nullptr;
    QOpenGLTexture *mTextureGrid=nullptr;

    QOpenGLVertexArrayObject mVAO;

    bool mLeftMouseButtonPressed = false;
    int mLeftMouseButtonPressCoordX;
    int mLeftMouseButtonPressCoordY;

    bool mDragState = false;

    QOpenGLShaderProgram mShader;

    int mSavedX = 0;
    int mSavedY = 0;

    QVector3D mEye;
    QVector3D mView;
    QVector3D mUp;

    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mOrthoProjectionMatrix;

    void initShaders();
    void initTextures();

    bool screenToWorld(int winx, int winy, int z_ndc, QVector3D &ray_world);
    float linearizeDepth(float depth);

Q_SIGNALS:
    void glInitialized();
    void leftMouseButtonPressed(QVector3D);
    void rightMouseButtonPressed(QVector3D);
    void mouseButtonClicked(QVector3D);
    void mouseMove(QVector3D);
    void dragStarted(QVector3D);
    void dropped(QVector3D);

public slots:
    void on_mTimer_timeout();

};

#endif // CGLWIDGET_H
