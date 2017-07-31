import yamale

def load_schema(path, parser):
    return yamale.make_schema(path, parser)

def load_data(path, parser):
    return yamale.make_data(path, parser)

