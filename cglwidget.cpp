#include "cglwidget.h"
#include <QDebug>
#include <QtOpenGL>

#include <qmath.h>

#include <locale.h>

#include <QString>
#include <QVectorIterator>

#include <QTimer>
#include "utils.h"

cGLWidget::cGLWidget(QWidget *parent) :
    QOpenGLWidget(parent), mEye(QVector3D(0,0,700)), mView(QVector3D(0,0,0)), mUp(QVector3D(0,1,0))
{
    setMouseTracking(true);

    mTimer = new QTimer(this);
    mTimer->setObjectName("mTimer");
    connect(mTimer, SIGNAL(timeout()), this, SLOT(on_mTimer_timeout()));
    mTimer->start(10);
}

float cGLWidget::scale(float from_min, float from_max, float to_min, float to_max, float val)
{
    return (to_max - to_min)*(val - from_min) / (from_max - from_min) + to_min;
}

bool cGLWidget::worldToScreen(const QVector3D &pw, int& x, int& y)
{
    QMatrix4x4 viewMatrix;
    viewMatrix.lookAt(mEye, mView, mUp);

    QVector4D clipSpacePos = mProjectionMatrix * (viewMatrix * QVector4D(pw, 1.0));
    QVector3D ndcSpacePos = clipSpacePos.toVector3D() / clipSpacePos.w();

    // Scale NDC to Screen Space
    x = qRound((ndcSpacePos.x() + 1.0) * 0.5 * width());
    y = qRound((-ndcSpacePos.y() + 1.0) * 0.5 * height());

    return true;
}

bool cGLWidget::screenToWorld(int winx, int winy, int z_ndc, QVector3D &pw)
{
    // define the plane
    sPlane plane(QVector3D(0,0,1), QVector3D(0,0,0));

    QMatrix4x4 viewMatrix;

    QVector4D ray_clip((2.0f * winx) / width() - 1.0f, 1.0f - (2.0f * winy) / height(), z_ndc, 1.0);
    QVector4D ray_eye = mProjectionMatrix.inverted() * ray_clip;

    ray_eye.setZ(-1);
    ray_eye.setW(0);

    viewMatrix.lookAt(mEye, mView, mUp);
    QVector3D rw = QVector4D(viewMatrix.inverted() * ray_eye).toVector3D();

    float c, d = QVector3D::dotProduct(plane.n, plane.p);

    QVector3D rayDir = (mView - mEye).normalized();

    if(mPerspectiveProjection) {
        c = QVector3D::dotProduct(plane.n, mEye) - d;
    } else {
        rw += mEye;
        c = QVector3D::dotProduct(plane.n, rw) - d;
    }

    if(c < 0) { return false; } // we are behind the plane

    float a;
    if(mPerspectiveProjection) {
        a = QVector3D::dotProduct(plane.n, rw);
    } else {
        a = QVector3D::dotProduct(plane.n, rayDir);
    }

    if(a==0) { return false; } // ray is parallel to the plane

    float t = c / a;

    if(t==0) return false;

    if(mPerspectiveProjection) {
        pw = mEye - rw * t;
    } else {
        pw = rw - rayDir * t;
    }

    return true;
}


void cGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    //glClearColor(0.2f, 0.6f, 0.2f, 1.f); // green
    glClearColor(0.9608f, 0.9608f, 0.8627f, 1.f); // beige

    qDebug() << "Supported shading language version"  << QString((char*)glGetString(GL_VENDOR))<< QString((char*)glGetString(GL_RENDERER)) << QString((char*)glGetString(GL_VERSION)) /*<< QString((char*)glGetString(GL_EXTENSIONS))*/ << QString((char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    qDebug() << "Initializing shaders...";

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    //glEnable(GL_CULL_FACE);
    //glEnable(GL_MULTISAMPLE); // qt surface format sets the number of sampling

    initShaders();

    glClearDepth(1.0); // Depth Buffer Setup
    glDepthFunc(GL_LEQUAL); // The Type Of Depth Testing To Do

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    mVAO.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&mVAO);

    emit(glInitialized());
}

void cGLWidget::initShaders()
{
    // Overriding system locale until shaders are compiled
    setlocale(LC_NUMERIC, "C");

    if(!mShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vs.glsl"))
    {
        qDebug() << "error compiling vertex shader";
        close();
    }

    if (!mShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fs.glsl"))
    {
        qDebug() << "error compiling fragment shader";
        close();
    }

    // Linking shader pipeline
    if (!mShader.link())
        close();

    // Binding shader pipeline for use
    if (!mShader.bind())
        close();

    mShader.release();

    // Restore system locale
    setlocale(LC_ALL, "");
}

void cGLWidget::clearVBOs()
{
    mVBOs.clear();
}

void cGLWidget::destroyVBO(const QString &name)
{
    if(mNamedVBOs.contains(name))
        mNamedVBOs[name].vbo.destroy();

    mNamedVBOs.remove(name);
}

void cGLWidget::addToVBO(const QString& name, const std::vector<GLfloat> &buffer, int mode, QVector4D lineColor)
{
    if(!buffer.size()) return;

    if(mNamedVBOs.contains(name))
        mNamedVBOs[name].vbo.destroy();

    sBufferWithColor b;
    b.mode = mode;
    b.lineColor = lineColor;
    b.vbo.create();
    b.vbo.bind();
    b.vbo.allocate(buffer.data(), buffer.size() * sizeof(GLfloat));
    b.vbo.release();

    if(mNamedVBOs.contains(name))
        mNamedVBOs[name] = b;
    else
        mNamedVBOs.insert(name, b);
}

void cGLWidget::addToVBO(const std::vector<GLfloat> &buffer, int mode, QVector4D lineColor)
{
    if(!buffer.size()) return;

    sBufferWithColor b;
    b.mode = mode;
    b.lineColor = lineColor;
    b.vbo.create();
    b.vbo.bind();
    b.vbo.allocate(buffer.data(), buffer.size() * sizeof(GLfloat));
    b.vbo.release();
    mVBOs.push_back(b);
}

void cGLWidget::topView()
{
    mEye = QVector3D(0,450,500);
    mView = QVector3D(0,0,0);
    mUp = QVector3D(0,1,0);

    update();
}

void cGLWidget::leftView()
{
    mEye = QVector3D(-700,0,350);
    mView = QVector3D(0,0,0);
    mUp = QVector3D(0,1,0);

    update();
}

void cGLWidget::rightView()
{
    mEye = QVector3D(700,0,350);
    mView = QVector3D(0,0,0);
    mUp = QVector3D(0,1,0);

    update();
}

void cGLWidget::frontView()
{
    mEye = QVector3D(0,0,700);
    mView = QVector3D(0,0,0);
    mUp = QVector3D(0,1,0);

    update();
}


void cGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w)
    Q_UNUSED(h)
    updateProjection(mPerspectiveProjection);
}

void cGLWidget::updateProjection(bool perspectiveProjection)
{
    float w=width(), h=height();

    float aspect = w / h;

    float os = 512.;

    mProjectionMatrix.setToIdentity();
    if(perspectiveProjection) {
        mProjectionMatrix.perspective(45.0f, aspect, 0.1f, 2000.0f);
    } else {
        if(w <= h)  {
            mProjectionMatrix.ortho(-os, os, -os/aspect, os/aspect, 1.0f, 1000.0f);
        } else {
            mProjectionMatrix.ortho(-os*aspect, os*aspect, -os, os, 1.0f, 1000.0f);
        }
        mOrthoProjectionMatrix = mProjectionMatrix;
    }
    mPerspectiveProjection = perspectiveProjection;
}

void cGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    QOpenGLVertexArrayObject::Binder vaoBinder(&mVAO);

    QOpenGLFunctions *f=nullptr;
    QMatrix4x4 viewMatrix, modelMatrix, lightViewMatrix;

    modelMatrix.setToIdentity();

    viewMatrix.lookAt(mEye, mView, mUp);

    for(auto& b : mVBOs) {
        b.vbo.bind();
        mShader.bind();

        mShader.setUniformValue("qt_lineColor", b.lineColor);
        mShader.setUniformValue("qt_modelViewMatrix", viewMatrix);
        mShader.setUniformValue("qt_projectionMatrix", mProjectionMatrix);

        f = ctx->functions();
        f->glEnableVertexAttribArray(0);

        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        glDrawArrays(b.mode, 0, b.vbo.size() / 3 / sizeof(GLfloat));

        mShader.release();
        b.vbo.release();
    }

    // draw named vbos
    for(auto k: mNamedVBOs.keys())
    {
        sBufferWithColor& b = mNamedVBOs[k];
        b.vbo.bind();
        mShader.bind();

        mShader.setUniformValue("qt_lineColor", b.lineColor);
        mShader.setUniformValue("qt_modelViewMatrix", viewMatrix);
        mShader.setUniformValue("qt_projectionMatrix", mProjectionMatrix);

        f = ctx->functions();
        f->glEnableVertexAttribArray(0);

        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        glDrawArrays(b.mode, 0, b.vbo.size() / 3 / sizeof(GLfloat));

        mShader.release();
        b.vbo.release();
    }

}

float cGLWidget::linearizeDepth(float depth)
{
    float zNear = 10;    // TODO: Replace by the zNear of your perspective projection
    float zFar  = 100.0; // TODO: Replace by the zFar  of your perspective projection
    return (2.0f * zNear) / (zFar + zNear - depth * (zFar - zNear));
}

void cGLWidget::keyPressEvent(QKeyEvent *ke)
{
    mKeyDown[ke->key()] = true;
}

void cGLWidget::keyReleaseEvent(QKeyEvent *ke)
{
    mKeyDown[ke->key()] = false;
}

void cGLWidget::on_mTimer_timeout()
{
    float speed = 3;
    QVector3D eyeDir = (mView-mEye).normalized();

    if(mKeyDown[Qt::Key_W])
    {
        mEye += eyeDir * speed;
        mView += eyeDir * speed;

        qDebug() << "eye" << mEye << "view" << mView << "up" << mUp;
    }
    if(mKeyDown[Qt::Key_S])
    {
        mEye += -eyeDir * speed;
        mView += -eyeDir * speed;

        qDebug() << "eye" << mEye << "view" << mView << "up" << mUp;
    }
    if(mKeyDown[Qt::Key_A])
    {
        QVector3D xaxis = QVector3D::crossProduct(mUp, eyeDir).normalized();

        mEye += xaxis * speed;
        mView += xaxis * speed;

        qDebug() << "eye" << mEye << "view" << mView << "up" << mUp;
    }
    if(mKeyDown[Qt::Key_D])
    {
        QVector3D xaxis = QVector3D::crossProduct(mUp, eyeDir).normalized();

        mEye += -xaxis * speed;
        mView += -xaxis * speed;

        qDebug() << "eye" << mEye << "view" << mView << "up" << mUp;
    }

    if(mKeyDown[Qt::Key_Q])
    {
        mEye += mUp * speed;
        mView += mUp * speed;
    }
    if(mKeyDown[Qt::Key_E])
    {
        mEye -= mUp * speed;
        mView -= mUp * speed;
    }

    update();
}

void cGLWidget::mouseMoveEvent(QMouseEvent *me)
{
    QVector3D pw;
    if(screenToWorld(me->x(), me->y(), -1.0, pw)) {
        Q_EMIT mouseMove(pw);
    }

    if(mLeftMouseButtonPressed) {
        if(mKeyDown[Qt::Key_Control]) { // look around in the scene
            QQuaternion result;

            QVector3D eyeDir = (mView-mEye).normalized();

            float xangle = d2r(mLeftMouseButtonPressCoordX - me->x()) * 0.1f;
            float yangle = -d2r(mLeftMouseButtonPressCoordY - me->y()) * 0.1f;

            mLeftMouseButtonPressCoordX = me->x();
            mLeftMouseButtonPressCoordY = me->y();

            QVector3D yaxis = QVector3D(0,1,0); // rotate around the world's up vector
            QQuaternion r_quat(cos(xangle/2), yaxis.x()*sin(xangle/2), yaxis.y()*sin(xangle/2), yaxis.z()*sin(xangle/2));
            r_quat.normalize();
            QQuaternion v_quat(0, eyeDir); // create view quaternion from view vector
            QQuaternion u_quat(0, mUp);
            result = (r_quat * v_quat) * r_quat.conjugate();
            QVector3D eyeShit = QVector3D(result.x(), result.y(), result.z()).normalized();
            mView = mEye + eyeShit;

            result = (r_quat * u_quat) * r_quat.conjugate();
            mUp = QVector3D(result.x(), result.y(), result.z()).normalized();
            qDebug()  << "eye" << mEye << "view" << mView << "up" << mUp << "eyeDir" << eyeDir;

            // new eyedir based on the previous rotation about the vertical camera axis
            eyeDir = (mView-mEye).normalized();

            // rotation axis
            QVector3D xaxis = QVector3D::crossProduct(mUp, eyeDir).normalized();

            QQuaternion rr_quat(cos(xangle/2), xaxis.x()*sin(yangle/2), xaxis.y()*sin(yangle/2), xaxis.z()*sin(yangle/2));
            rr_quat.normalize();
            QQuaternion vv_quat(0, eyeDir); // create view quaternion from view vector
            QQuaternion uu_quat(0, mUp);
            result = (rr_quat * vv_quat) * rr_quat.conjugate();
            eyeShit = QVector3D(result.x(), result.y(), result.z()).normalized();
            mView = mEye + eyeShit;

            result = (rr_quat * uu_quat) * rr_quat.conjugate();
            mUp = QVector3D(result.x(), result.y(), result.z()).normalized();

            qd << "quat res" << result;

            update();
        } else { // drag started
            if(!mDragState) {
                mDragState = true;
                Q_EMIT(dragStarted(pw));
            }
        }



    }


    update();
}

void cGLWidget::mousePressEvent(QMouseEvent *me)
{
    if(me->button() == Qt::LeftButton) {
        // this is for navigation
        mLeftMouseButtonPressed = true;
        mLeftMouseButtonPressCoordX = me->x();
        mLeftMouseButtonPressCoordY = me->y();

        // this is for mouse click signal
        mSavedX = me->x();
        mSavedY = me->y();
    }

    QVector3D pw;
    if(screenToWorld(me->x(), me->y(), -1.0, pw)) {
        if(me->button() == Qt::LeftButton)
            Q_EMIT leftMouseButtonPressed(pw);
        else
            Q_EMIT rightMouseButtonPressed(pw);
    }

    update();
}

void cGLWidget::mouseReleaseEvent(QMouseEvent *me)
{
    QVector3D pw;
    bool res = screenToWorld(me->x(), me->y(), -1.0, pw);

    if(me->button() == Qt::LeftButton) {

        if(mLeftMouseButtonPressed) {
            if(mSavedX == me->x() && mSavedY == me->y()) {
                if(res) {
                    Q_EMIT mouseButtonClicked(pw);
                }
            }
        }

        mLeftMouseButtonPressed = false;

        if(mDragState) {
            if(res) {
                Q_EMIT dropped(pw);
            }
        }
        mDragState = false;

    }
}
