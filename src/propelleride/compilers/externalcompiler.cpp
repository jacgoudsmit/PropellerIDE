#include "externalcompiler.h" 

#include <QDebug>
#include <QDir>
#include <QString>
#include <QRegularExpression>
#include <QSettings>
#include <QFileInfo>

ExternalCompiler::ExternalCompiler(QString name, QObject * parent)
    : Compiler(parent)
{
    proc = NULL;

//    save("compiler.bstc");
//    load(":/config/compiler.bstc");

    arg_exe = name;
    QString filename = ":/config/compiler."+arg_exe;

    load(filename);
}

ExternalCompiler::~ExternalCompiler()
{
    cleanup();
}

QString ExternalCompiler::build(QString filename,
                                QStringList libraries)
{
    qDebug() << "Building project" << filename;

    QString infile = filename;

    if (!infile.endsWith(pattern_in, Qt::CaseInsensitive))
    {
        qCritical() << "Input file extension does not equal" << pattern_in;
        return QString();
    }

    QString basefile = infile;
    basefile.chop(pattern_in.size());

    qDebug() << "BASE" << basefile;

    QString outfile = basefile.append(pattern_out);
    QString retfile = basefile.append(pattern_ret);

    qDebug() << "IN" << infile;
    qDebug() << "OUT" << outfile;
    qDebug() << "RET" << retfile;

    QStringList arglist;

    arglist << arg_exe << infile;

    if (!arg_output.isEmpty())
    {
        arglist << arg_output.replace("%",outfile);
    }

    if (!arg_flags.isEmpty())
    {
        foreach(QString flag, arg_flags.split(" "))
        {
            if (!flag.trimmed().isEmpty())
                arglist << flag;
        }
    }

    if (!arg_library.isEmpty())
    {
        foreach (QString l, libraries)
        {
            if (l.size())
                arglist << "-L"+l;
        }
    }

    if (arglist.isEmpty())
    {
        qCritical() << "No compiler specified";
        return QString();
    }

    QString program = QDir::toNativeSeparators(arglist.takeFirst());

    QStringList args;
    for (int i = 0; i < arglist.size(); ++i)
    {
        args.append(QDir::toNativeSeparators(arglist.at(i)));
    }

    qDebug() << qPrintable(program);
    foreach (QString a, args)
    {
        qDebug() << "    " << qPrintable(a);
    }

    if (proc)
    {
        qCritical() << "Build already in progress!";
        return QString();
    }

    proc = new QProcess();
    proc->setProcessChannelMode(QProcess::MergedChannels);

    connect(proc,   SIGNAL(readyReadStandardOutput()),          this,   SLOT(readOutput()));
    connect(proc,   SIGNAL(error(QProcess::ProcessError)),      this,   SLOT(handleError(QProcess::ProcessError)));
    connect(proc,   SIGNAL(finished(int,QProcess::ExitStatus)), this,   SLOT(finish(int,QProcess::ExitStatus)));
    connect(this,   SIGNAL(finished(bool)),                     this,   SLOT(cleanup()));

    proc->start(program, args);

    return retfile;
}

void ExternalCompiler::readOutput()
{
    QString output = QString(proc->readAllStandardOutput());
    if (output.isEmpty())
        return;

    output = output.replace("\r\n", "\n");
    print(output);

    QStringList lines = output.split("\n", QString::SkipEmptyParts);
    foreach (QString line, lines)
    {
        QRegularExpressionMatch m_error = re_error.match(line);
        QRegularExpressionMatch m_success = re_success.match(line);

        if (m_error.hasMatch())
        {
            qCritical() << qPrintable(QString("%1 (%2, %3): %4")
                            .arg(m_error.captured(1))
                            .arg(m_error.captured(2))
                            .arg(m_error.captured(3))
                            .arg(m_error.captured(4)));
            emit highlightLine(m_error.captured(1)+".spin",
                               m_error.captured(2).toInt()-1,
                               m_error.captured(3).toInt()-1,
                               m_error.captured(4));
            return;
        }
        else if (m_success.hasMatch())
        {
            qDebug() << m_success.captured(1);
        }

    }
}

void ExternalCompiler::finish(int exitCode, QProcess::ExitStatus status)
{
    if (exitCode != 0 || status != QProcess::NormalExit)
        emit finished(false);
    else
        emit finished(true);
}

void ExternalCompiler::handleError(QProcess::ProcessError e)
{
    QString errorstring;
    switch (e)
    {
        case QProcess::FailedToStart:
            errorstring = tr("Failed to start compiler: '%1'; check Preferences").arg("compiler");
            break;
        case QProcess::Crashed:
        case QProcess::Timedout:
        case QProcess::WriteError:
        case QProcess::ReadError:
        case QProcess::UnknownError:
            errorstring = proc->errorString();
            break;
    }

    qCritical() << errorstring;
//    print("ERROR: "+errorstring,Qt::red);
//    QMessageBox::critical((QWidget *) parent(),
//            tr("Build Failed"),
//            tr("%1").arg(errorstring));

    emit finished(false);
}

void ExternalCompiler::cleanup()
{
    if (proc)
        delete proc;
    proc = NULL;
}

void ExternalCompiler::load(QString filename)
{
    QSettings settings(filename, QSettings::IniFormat);

    arg_flags   = settings.value("ARG_FLAGS").toString();
    arg_library = settings.value("ARG_LIBRARY").toString();
    arg_output  = settings.value("ARG_OUTPUT").toString();

    pattern_in  = settings.value("PATTERN_IN").toString();
    pattern_out = settings.value("PATTERN_OUT").toString();
    pattern_ret = settings.value("PATTERN_RET").toString();

    re_success  = QRegularExpression(settings.value("RE_SUCCESS").toString(),
                                     QRegularExpression::CaseInsensitiveOption);
    re_error    = QRegularExpression(settings.value("RE_ERROR").toString(),
                                     QRegularExpression::CaseInsensitiveOption);
}

void ExternalCompiler::save(QString filename)
{
    QSettings settings(filename, QSettings::IniFormat);

    settings.setValue("ARG_FLAGS",      "-b -Oucrg");
    settings.setValue("ARG_LIBRARY",    "-L%");
    settings.setValue("ARG_OUTPUT",     "-o%");

    settings.setValue("PATTERN_IN",     "%.spin");
    settings.setValue("PATTERN_OUT",    "%.binary");
    settings.setValue("PATTERN_RET",    "%.binary");

    settings.setValue("RE_SUCCESS",     "(Program size is)");
    settings.setValue("RE_ERROR",       "(.*?)\\s*\\(\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*\\)\\s*error\\s*:\\s*(.*)");
}
