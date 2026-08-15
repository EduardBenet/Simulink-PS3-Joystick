function plan = buildfile
import matlab.buildtool.tasks.*

plan = buildplan(localfunctions);

plan("clean") = CleanTask;
plan("check") = CodeIssuesTask('src', 'WarningThreshold',0, 'InfoThreshold',0);

plan("archive").Dependencies = ["clean", "check"];

plan.DefaultTasks = "archive";
end

function archiveTask(~)

v = ver('ps3controller');

opts = matlab.addons.toolbox.ToolboxOptions('src', "14407be0-3ccd-4fa1-8fff-d2d121ef112d");
opts.AuthorCompany = "MathWorks";
opts.AuthorEmail = "ebenetcerda@gmail.com";
opts.AuthorName = "Eduard Benet Cerda";
opts.Description = "A simulink block for a PS3 controller";
opts.OutputFile = fullfile(currentProject().RootFolder, 'releases', 'ps3controller.mltbx');
opts.Summary = "ps3controller library";
opts.ToolboxName = "PS3Controller";
opts.ToolboxVersion = v.Version;

matlab.addons.toolbox.packageToolbox(opts);

end